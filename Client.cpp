#include "Common.h"
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>

#define SERVER_IP "121.184.194.12"  // ������ IP �ּ�
#define SERVER_PORT 8080

// ��Ʈ��ũ ���� �Լ�
SOCKET setup_network() {
    WSADATA wsaData;
    SOCKET clientSocket;
    SOCKADDR_IN serverAddr;

    // Winsock �ʱ�ȭ
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        err_quit("WSAStartup() ����");
    }

    // ���� ����
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        err_quit("socket() ����");
    }

    // ���� �ּ� ����
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    serverAddr.sin_port = htons(SERVER_PORT);

    // ������ ����
    if (connect(clientSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        err_quit("connect() ����");
    }

    printf("������ ����Ǿ����ϴ�.\n");

    return clientSocket;
}

// ��ǰ ���� �Լ�
int choose_beverage() {
    int choice;
    printf("=============================\n");
    printf("[1. �� - 450��\t\t]\n");
    printf("[2. Ŀ�� - 500��\t]\n");
    printf("[3. �̿����� - 550��\t]\n");
    printf("[4. ���Ŀ�� - 700��\t]\n");
    printf("[5. ź������ - 750��\t]\n");
    printf("[6. ��ǰ��� Ȯ��\t]\n");
    printf("��ǰ ��ȣ�� �����ϼ���:  ");
    printf("\n=============================\n");
    scanf("%d", &choice);
    return choice;
}

// �ݾ� ���� �Լ�
int charge(int totalAmount) {
    int count = 0;
    int amount;

    while (1) {
        printf("���� �ݾ�: %d��\n", totalAmount);
        printf("�ݾ��� �����Ͻðڽ��ϱ�? |10, 50, 100, 500, 1000�� ����, 0�� �Է��ϸ� ����| : ");
        scanf("%d", &amount);

        if (amount == 0) {
            break;
        }

        if (amount == 1000) {
            count++;
        }

        if (amount != 10 && amount != 50 && amount != 100 && amount != 500 && amount != 1000) {
            printf("��ȿ���� ���� ȭ�� �����Դϴ�. �ٽ� �Է����ּ���.\n");
            continue;
        }

        if (count > 3 && amount == 1000) {
            printf("�����ʰ�. �ٽ� �Է����ּ���.\n");
            continue;
        }

        totalAmount += amount;
        if (totalAmount > 5000) {
            printf("5000�� �̻� ������ �����ϴ�. �ٽ� �Է����ּ���.\n");
            totalAmount -= amount;  // �ʰ��� �ݾ��� �ٽ� ����
            continue;
        }
    }

    return totalAmount;
}

void communicate_with_server(SOCKET clientSocket, int choice, int* totalAmount) {
    char sendBuffer[2000];
    char recvBuffer[2000];

    // ������ ������ ����
    sprintf(sendBuffer, "%d,%d", choice, *totalAmount);
    if (send(clientSocket, sendBuffer, strlen(sendBuffer), 0) == SOCKET_ERROR) {
        err_display("send() ����");
        return;
    }

    // �����κ��� ���� ����
    memset(recvBuffer, 0, sizeof(recvBuffer));
    int recvSize;
    int retries = 3; // ��õ� Ƚ��
    do {
        recvSize = recv(clientSocket, recvBuffer, sizeof(recvBuffer) - 1, 0);
        if (recvSize == SOCKET_ERROR) {
            err_display("recv() ����");
            retries--;
            if (retries > 0) {
                printf("��õ� ��...\n");
                Sleep(1000); // 1�� ��� �� ��õ�
            }
        }
        else if (recvSize == 0) {
            printf("Ŭ���̾�Ʈ�� ������ �����߽��ϴ�.\n");
            break;
        }
    } while (recvSize == SOCKET_ERROR && retries > 0);

    if (recvSize != SOCKET_ERROR) {
        printf("�����κ��� ���� ����: %s\n", recvBuffer);

        // �Ž����� ����
        int change;
        sscanf(recvBuffer, "%*[^0-9]%d", &change);

        // choice ���� 9�� �ƴ� ��쿡�� ���� �ݾ׿� �ݿ�
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
        printf("********************�������� ���Ǳ�********************\n\n");
        // ����
        totalAmount = charge(totalAmount);

        // ��ǰ ���� �� ���
        int choice = choose_beverage();
        communicate_with_server(clientSocket, choice, &totalAmount); // �μ��� int* �������� ����

        // ��ǰ ���� �� ����
        int option = post_continue();

        if (option == 0) {   // ���� ����
            break;
        }
    }

    // Ŭ���̾�Ʈ ���� ����
    closesocket(clientSocket);

    // Winsock ����
    WSACleanup();

    return 0;
}