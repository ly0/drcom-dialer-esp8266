#include "esp_common.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "md5.h"
#include "udhcp/dhcpd.h"

char drcom_user[33];
char drcom_pass[17];
uint64_t drcom_mac;
uint64_t mac_test = 0xDC0EA1218AD6;

char host[] = "drcom";
char os[] = "drcom";
int user_len;
int pass_len;
int host_len = sizeof(host) - 1;
int os_len = sizeof(os) - 1;


//SERVER_DOMAIN login.jlu.edu.cn
#define DRCOM_SERVER_ADDR "10.100.61.3"
#define DRCOM_SERVER_PORT 61440

#define RECV_DATA_SIZE 1000
#define SEND_DATA_SIZE 1000
#define CHALLENGE_TRY 10
#define LOGIN_TRY 5
#define ALIVE_TRY 5

/* infomation */
struct user_info_pkt {
    char *username;
    char *password;
    char *hostname;
    char *os_name;
    uint64_t mac_addr;
    int username_len;
    int password_len;
    int hostname_len;
    int os_name_len;
};

/* signal process flag */
int logout_flag = 0;

void ICACHE_FLASH_ATTR
get_user_info(struct user_info_pkt *user_info)
{
    user_info->username = drcom_user;
    user_info->username_len = user_len;
    user_info->password = drcom_pass;
    user_info->password_len = pass_len;
    user_info->hostname = host;
    user_info->hostname_len = host_len;
    user_info->os_name = os;
    user_info->os_name_len = os_len;
    user_info->mac_addr = drcom_mac;
}

void ICACHE_FLASH_ATTR
set_challenge_data(unsigned char *clg_data, int clg_data_len, int clg_try_count)
{
    /* set challenge */
    int random = rand() % 0xF0 + 0xF;
    int data_index = 0;
    memset(clg_data, 0x00, clg_data_len);
    /* 0x01 challenge request */
    clg_data[data_index++] = 0x01;
    /* clg_try_count first 0x02, then increment */
    clg_data[data_index++] = 0x02 + (unsigned char)clg_try_count;
    /* two byte of challenge_data */
    clg_data[data_index++] = (unsigned char)(random % 0xFFFF);
    clg_data[data_index++] = (unsigned char)((random % 0xFFFF) >> 8);
    /* end with 0x09 */
    clg_data[data_index++] = 0x09;
}

void ICACHE_FLASH_ATTR
challenge(int sock, struct sockaddr_in serv_addr, unsigned char *clg_data, int clg_data_len, char *recv_data, int recv_len)
{
    int ret;
    int challenge_try = 0;
    do {
        if (challenge_try > CHALLENGE_TRY) {
            //close(sock);
            printf("[drcom-challenge]: try challenge, but failed, please check your network connection.\n");
        }
        set_challenge_data(clg_data, clg_data_len, challenge_try);
        challenge_try++;
        ret = sendto(sock, clg_data, clg_data_len, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
        if (ret != clg_data_len) {
            printf("[drcom-challenge]: send challenge data failed.\n");
            continue;
        }
        ret = recvfrom(sock, recv_data, recv_len, 0, NULL, NULL);
        if (ret < 0) {
            printf("[drcom-challenge]: recieve data from server failed.\n");
            continue;
        }
        if (*recv_data != 0x02) {
            if (*recv_data == 0x07) {
                close(sock);
                printf("[drcom-challenge]: wrong challenge data.\n");
            }
            printf("[drcom-challenge]: challenge failed!, try again.\n");
        }
    } while ((*recv_data != 0x02));
    printf("[drcom-challenge]: challenge success!\n");
}

void ICACHE_FLASH_ATTR
set_login_data(struct user_info_pkt *user_info, unsigned char *login_data, int login_data_len, unsigned char *salt, int salt_len)
{
    /*
void ICACHE_FLASH_ATTR
get_user_info(struct user_info_pkt *user_info)
{
    user_info->username = drcom_user;
    user_info->username_len = user_len;
    user_info->password = drcom_pass;
    user_info->password_len = pass_len;
    user_info->hostname = host;
    user_info->hostname_len = host_len;
    user_info->os_name = os;
    user_info->os_name_len = os_len;
    user_info->mac_addr = drcom_mac;
}
    */
    printf("[*] will login: username=%s, password=%s, mac=", user_info->username, user_info->password);
    print_data(&user_info->mac_addr, 6);
    printf("\n\n\n");
    /* login data */
    int i, j;
    unsigned char md5_str[16];
    unsigned char md5_str_tmp[100];
    int md5_str_len;

    int data_index = 0;

    memset(login_data, 0x00, login_data_len);

    /* magic 3 byte, username_len 1 byte */
    login_data[data_index++] = 0x03;
    login_data[data_index++] = 0x01;
    login_data[data_index++] = 0x00;
    login_data[data_index++] = (unsigned char)(user_info->username_len + 20);

    /* md5 0x03 0x01 salt password */
    md5_str_len = 2 + salt_len + user_info->password_len;
    memset(md5_str_tmp, 0x00, md5_str_len);
    md5_str_tmp[0] = 0x03;
    md5_str_tmp[1] = 0x01;
    memcpy(md5_str_tmp +2, salt, salt_len);
    memcpy(md5_str_tmp + 2 + salt_len, user_info->password, user_info->password_len);
    MD5(md5_str_tmp, md5_str_len, md5_str);
    memcpy(login_data + data_index, md5_str, 16);
    data_index += 16;

    /* user name 36 */
    memcpy(login_data + data_index, user_info->username, user_info->username_len);
    data_index += user_info->username_len > 36 ? user_info->username_len : 36;

    /* 0x00 0x00 */
    data_index += 2;

    /* (data[4:10].encode('hex'),16)^mac */
    uint64_t sum = 0;
    for (i = 0; i < 6; i++) {
        sum = (int)md5_str[i] + sum * 256;
    }
    sum ^= user_info->mac_addr;
    for (i = 6; i > 0; i--) {
        login_data[data_index + i - 1] = (unsigned char)(sum % 256);
        sum /= 256;
    }
    data_index += 6;
        
    /* md5 0x01 pwd salt 0x00 0x00 0x00 0x00 */
    md5_str_len = 1 + user_info->password_len + salt_len + 4;
    memset(md5_str_tmp, 0x00, md5_str_len);
    md5_str_tmp[0] = 0x01;
    memcpy(md5_str_tmp + 1, user_info->password, user_info->password_len);
    memcpy(md5_str_tmp + 1 + user_info->password_len, salt, salt_len);
    MD5(md5_str_tmp, md5_str_len, md5_str);
    memcpy(login_data + data_index, md5_str, 16);
    data_index += 16;

    /* 0x01 0x31 0x8c 0x21 0x28 0x00*12 */
    login_data[data_index++] = 0x01;
    login_data[data_index++] = 0x31;
    login_data[data_index++] = 0x8c;
    login_data[data_index++] = 0x21;
    login_data[data_index++] = 0x28;
    data_index += 12;

    /* md5 login_data[0-data_index] 0x14 0x00 0x07 0x0b 8 bytes */
    md5_str_len = data_index + 4;
    memset(md5_str_tmp, 0x00, md5_str_len);
    memcpy(md5_str_tmp, login_data, data_index);
    md5_str_tmp[data_index+0] = 0x14;
    md5_str_tmp[data_index+1] = 0x00;
    md5_str_tmp[data_index+2] = 0x07;
    md5_str_tmp[data_index+3] = 0x0b;
    MD5(md5_str_tmp, md5_str_len, md5_str);
    memcpy(login_data + data_index, md5_str, 8);
    data_index += 8;

    /* 0x01 0x00*4 */
    login_data[data_index++] = 0x01;
    data_index += 4;

    /* hostname */
    i = user_info->hostname_len > 71 ? 71 : user_info->hostname_len;
    memcpy(login_data + data_index, user_info->hostname, i);
    data_index += 71;

    /* 0x01 */
    login_data[data_index++] = 0x01;

    /* osname */
    i = user_info->os_name_len > 128 ? 128 : user_info->os_name_len;
    memcpy(login_data + data_index, user_info->os_name, i);
    data_index += 128;

    /* 0x6d 0x00 0x00 len(pass) */
    login_data[data_index++] = 0x6d;
    login_data[data_index++] = 0x00;
    login_data[data_index++] = 0x00;
    login_data[data_index++] = (unsigned char)(user_info->password_len);

    /* ror (md5 0x03 0x01 salt pass) pass */
    md5_str_len = 2 + salt_len + user_info->password_len;
    memset(md5_str_tmp, 0x00, md5_str_len);
    md5_str_tmp[0] = 0x03;
    md5_str_tmp[1] = 0x01;
    memcpy(md5_str_tmp +2, salt, salt_len);
    memcpy(md5_str_tmp + 2 + salt_len, user_info->password, user_info->password_len);
    MD5(md5_str_tmp, md5_str_len, md5_str);
    int ror_check = 0;
    for (i = 0; i < user_info->password_len; i++) {
        ror_check = (int)md5_str[i] ^ (int)(user_info->password[i]);
        login_data[data_index++] = (unsigned char)(((ror_check << 3) & 0xFF) + (ror_check >> 5));
    }

    /* 0x02 0x0c */
    login_data[data_index++] = 0x02;
    login_data[data_index++] = 0x0c;

    /* checksum point */
    int check_point = data_index;
    login_data[data_index++] = 0x01;
    login_data[data_index++] = 0x26;
    login_data[data_index++] = 0x07;
    login_data[data_index++] = 0x11;

    /* 0x00 0x00 mac */
    login_data[data_index++] = 0x00;
    login_data[data_index++] = 0x00;
    uint64_t mac = user_info->mac_addr;
    for (i = 0; i < 6; i++) {
        login_data[data_index + i - 1] = (unsigned char)(mac % 256);
        mac /= 256;
    }
    data_index += 6;

    /* 0x00 0x00 0x00 0x00 the last two byte I dont't know*/
    login_data[data_index++] = 0x00;
    login_data[data_index++] = 0x00;
    login_data[data_index++] = 0x00;
    login_data[data_index++] = 0x00;

    /* checksum */
    sum = 1234;
    uint64_t check = 0;
    for (i = 0; i < data_index; i += 4) {
        check = 0;
        for (j = 0; j < 4; j++) {
            check = check * 256 + (int)login_data[i+j];
        }
        sum ^= check;
    }
    sum = (1968 * sum) & 0xFFFFFFFF;
    for (j = 0; j < 4; j++) {
        login_data[check_point+j] = (unsigned char)(sum >> (j*8) & 0x000000FF);
    }
}

void ICACHE_FLASH_ATTR
login(int sock, struct sockaddr_in serv_addr, unsigned char *login_data, int login_data_len, char *recv_data, int recv_len)
{
    /* login */
    int ret = 0;
    int login_try = 0;
    do {
        if (login_try > LOGIN_TRY) {
            close(sock);
            printf("[drcom-login]: try login, but failed, something wrong.\n");
        }
        login_try++;
        ret = sendto(sock, login_data, login_data_len, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
        if (ret != login_data_len) {
            printf("[drcom-login]: send login data failed.\n");
            continue;
        }
        ret = recvfrom(sock, recv_data, recv_len, 0, NULL, NULL);
        if (ret < 0) {
            printf("[drcom-login]: recieve data from server failed.\n");
            continue;
        }
        if (*recv_data != 0x04) {
            if (*recv_data == 0x05) {
                print_data(recv_data, recv_len);
                close(sock);
                printf("[drcom-login]: wrong password or username!\n\n");
            }
            printf("[drcom-login]: login failed!, try again\n");
        }
    } while ((*recv_data != 0x04));
    printf("[drcom-login]: login success!\n");
}

void ICACHE_FLASH_ATTR
set_alive_data(unsigned char *alive_data, int alive_data_len, unsigned char *tail, int tail_len, int alive_count, int random)
{
    // 0: 84 | 1: 82 | 2: 82 
    int i = 0;
    memset(alive_data, 0x00, alive_data_len);
    alive_data[i++] = 0x07;
    alive_data[i++] = (unsigned char)alive_count;
    alive_data[i++] = 0x28;
    alive_data[i++] = 0x00;
    alive_data[i++] = 0x0b;
    alive_data[i++] = (unsigned char)(alive_count * 2 + 1);
    alive_data[i++] = 0xdc;
    alive_data[i++] = 0x02;
    random += rand() % 10;
    for (i = 9 ; i > 7; i--) {
        alive_data[i] = random % 256; 
        random /= 256;
    }
    memcpy(alive_data+16, tail, tail_len);
    i = 25;
}


void ICACHE_FLASH_ATTR
main(void* args)
{
	printf("main \n");
    int sock, ret;
    unsigned char send_data[SEND_DATA_SIZE];
    char recv_data[RECV_DATA_SIZE];
    struct sockaddr_in serv_addr;
    struct user_info_pkt user_info;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        printf("[drcom]: create sock failed.\n");
    }

    bzero(&serv_addr, sizeof(struct sockaddr_in));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(DRCOM_SERVER_ADDR);
    serv_addr.sin_port = htons(DRCOM_SERVER_PORT);

    // get user information
    get_user_info(&user_info);

    // challenge data length 20
    challenge(sock, serv_addr, send_data, 20, recv_data, RECV_DATA_SIZE);

    // login data length 338, salt length 4
    set_login_data(&user_info, send_data, 338, (unsigned char *)(recv_data + 4), 4);
    memset(recv_data, 0x00, RECV_DATA_SIZE);
    login(sock, serv_addr, send_data, 338, recv_data, RECV_DATA_SIZE);

    // keep alive alive data length 42 or 40
    unsigned char tail[4];
    int tail_len = 4;
    memset(tail, 0x00, tail_len);
    int random = rand() % 0xFFFF;

    int alive_data_len = 0;
    int alive_count = 0;
    int alive_fail_count = 0;
    do { 
        if (alive_fail_count > ALIVE_TRY) {
            close(sock);
        }
        alive_data_len = alive_count > 0 ? 40 : 42;
        set_alive_data(send_data, alive_data_len, tail, tail_len, alive_count, random);
        ret = sendto(sock, send_data, alive_data_len, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
        if (ret != alive_data_len) {
            alive_fail_count++;
            continue;
        } else {
            alive_fail_count = 0;
        }
        memset(recv_data, 0x00, RECV_DATA_SIZE);
        ret = recvfrom(sock, recv_data, RECV_DATA_SIZE, 0, NULL, NULL);
        if (ret < 0 || *recv_data != 0x07) {
            alive_fail_count++;
            continue;
        } else {
            alive_fail_count = 0;
        }
        if (alive_count > 1) memcpy(tail, recv_data+16, tail_len);
        vTaskDelay(15000 / portTICK_RATE_MS);
        //sleep(15);
        alive_count = (alive_count + 1) % 3;
    } while (logout_flag != 1);

    return;
}
