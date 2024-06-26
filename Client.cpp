#include "Common.h"
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>

#define SERVER_IP "121.184.194.12"  // 서버의 IP 주소
#define SERVER_PORT 8080

// 네트워크 설정 함수
SOCKET setup_network() {
    WSADATA wsaData;
    SOCKET clientSocket;
    SOCKADDR_IN serverAddr;

    // Winsock 초기화
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        err_quit("WSAStartup() 오류");
    }

    // 소켓 생성
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        err_quit("socket() 오류");
    }

    // 서버 주소 설정
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    serverAddr.sin_port = htons(SERVER_PORT);

    // 서버에 연결
    if (connect(clientSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        err_quit("connect() 오류");
    }

    printf("서버에 연결되었습니다.\n");

    return clientSocket;
}

// 상품 선택 함수
int choose_beverage() {
    int choice;
    printf("=============================\n");
    printf("[1. 물 - 450원\t\t]\n");
    printf("[2. 커피 - 500원\t]\n");
    printf("[3. 이온음료 - 550원\t]\n");
    printf("[4. 고급커피 - 700원\t]\n");
    printf("[5. 탄산음료 - 750원\t]\n");
    printf("[6. 상품재고 확인\t]\n");
    printf("상품 번호를 선택하세요:  ");
    printf("\n=============================\n");
    scanf("%d", &choice);
    return choice;
}

// 금액 충전 함수
int charge(int totalAmount) {
    int count = 0;
    int amount;

    while (1) {
        printf("현재 금액: %d원\n", totalAmount);
        printf("금액을 충전하시겠습니까? |10, 50, 100, 500, 1000원 단위, 0을 입력하면 종료| : ");
        scanf("%d", &amount);

        if (amount == 0) {
            break;
        }

        if (amount == 1000) {
            count++;
        }

        if (amount != 10 && amount != 50 && amount != 100 && amount != 500 && amount != 1000) {
            printf("유효하지 않은 화폐 단위입니다. 다시 입력해주세요.\n");
            continue;
        }

        if (count > 3 && amount == 1000) {
            printf("지폐초과. 다시 입력해주세요.\n");
            continue;
        }

        totalAmount += amount;
        if (totalAmount > 5000) {
            printf("5000원 이상 넣을수 없습니다. 다시 입력해주세요.\n");
            totalAmount -= amount;  // 초과한 금액을 다시 차감
            continue;
        }
    }

    return totalAmount;
}

void communicate_with_server(SOCKET clientSocket, int choice, int* totalAmount) {
    char sendBuffer[2000];
    char recvBuffer[2000];

    // 서버로 데이터 전송
    sprintf(sendBuffer, "%d,%d", choice, *totalAmount);
    if (send(clientSocket, sendBuffer, strlen(sendBuffer), 0) == SOCKET_ERROR) {
        err_display("send() 오류");
        return;
    }

    // 서버로부터 응답 수신
    memset(recvBuffer, 0, sizeof(recvBuffer));
    int recvSize;
    int retries = 3; // 재시도 횟수
    do {
        recvSize = recv(clientSocket, recvBuffer, sizeof(recvBuffer) - 1, 0);
        if (recvSize == SOCKET_ERROR) {
            err_display("recv() 오류");
            retries--;
            if (retries > 0) {
                printf("재시도 중...\n");
                Sleep(1000); // 1초 대기 후 재시도
            }
        }
        else if (recvSize == 0) {
            printf("클라이언트가 연결을 종료했습니다.\n");
            break;
        }
    } while (recvSize == SOCKET_ERROR && retries > 0);

    if (recvSize != SOCKET_ERROR) {
        printf("서버로부터 받은 응답: %s\n", recvBuffer);

        // 거스름돈 추출
        int change;
        sscanf(recvBuffer, "%*[^0-9]%d", &change);

        // choice 값이 9가 아닌 경우에만 현재 금액에 반영
        if (choice != 9) {
            *totalAmount = change;
        }
    }
}

int post_continue() {
    int choice;
    printf("continue? (1: Yes, 0: No): ");
    scanf("%d", &choice);
    return choice;
}

int main() {
    SOCKET clientSocket;
    int totalAmount = 0;
    while (true) {
        clientSocket = setup_network();
        printf("********************성준이의 자판기********************\n\n");
        // 충전
        totalAmount = charge(totalAmount);

        // 상품 선택 후 통신
        int choice = choose_beverage();
        communicate_with_server(clientSocket, choice, &totalAmount); // 인수를 int* 형식으로 전달

        // 상품 구매 후 선택
        int option = post_continue();

        if (option == 0) {   // 구매 종료
            break;
        }
    }

    // 클라이언트 소켓 종료
    closesocket(clientSocket);

    // Winsock 종료
    WSACleanup();

    return 0;
}