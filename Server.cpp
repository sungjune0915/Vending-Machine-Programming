#include "Common.h"
#include <thread>
#include <string>
#include <ctime>
#include <map>
#include <sstream>
#include <fstream>

#define SERVER_PORT 8080
#define MAX_PRODUCTS 5
#define MAX_CLIENTS 10

struct ClientContext {
    SOCKET clientSocket;
    SOCKADDR_IN clientAddr;
};

static int inventory[MAX_PRODUCTS] = { 3, 3, 3, 3, 3 };
static int totalsale = 0;
static std::map<std::string, int> dailySales;
static std::map<std::string, int> monthlySales;

// 관리자 비밀번호
std::string adminPassword = "1111";

void ClientRequest(ClientContext* clientContext) {
    SOCKET clientSocket = clientContext->clientSocket;
    SOCKADDR_IN clientAddr = clientContext->clientAddr;
    char recvBuffer[2000];
    char sendBuffer[2000];

    // 클라이언트로부터 데이터 받기
    memset(recvBuffer, 0, sizeof(recvBuffer));
    int recvSize = recv(clientSocket, recvBuffer, sizeof(recvBuffer) - 1, 0);
    if (recvSize == SOCKET_ERROR) {
        err_display("recv() error");
        closesocket(clientSocket);
        delete clientContext;
        return;
    }

   
    int choice, totalAmount;
    sscanf(recvBuffer, "%d,%d", &choice, &totalAmount);

    if (choice == 6) {
        // 메뉴와 재고 보여주기
        sprintf(sendBuffer, "Inventory: 물 (%d), 커피 (%d), 이온 음료 (%d), 고급 커피 (%d), 탄산 음료 (%d)",
            inventory[0], inventory[1], inventory[2], inventory[3], inventory[4]);
    }
    else if (choice >= 1 && choice <= MAX_PRODUCTS) {
        int price = 0;
        const char* product = nullptr;

        switch (choice) {
        case 1:
            price = 450;
            product = "물";
            break;
        case 2:
            price = 500;
            product = "커피";
            break;
        case 3:
            price = 550;
            product = "이온 음료";
            break;
        case 4:
            price = 700;
            product = "고급 커피";
            break;
        case 5:
            price = 750;
            product = "탄산 음료";
            break;
        default:
            strcpy(sendBuffer, "Invalid selection.");
            break;
        }

        if (price > 0) {
            if (inventory[choice - 1] > 0) {
                // Reduce the inventory count by 1
                // Update daily sales data
                std::time_t now = std::time(nullptr);
                std::tm* now_tm = std::localtime(&now);
                char date[11];
                std::strftime(date, sizeof(date), "%Y-%m-%d", now_tm);
                dailySales[date] += price;

                char month[8];
                std::strftime(month, sizeof(month), "%Y-%m", now_tm);
                monthlySales[month] += price;

                if (totalAmount >= price) {
                    inventory[choice - 1]--;
                    int change = totalAmount - price;
                    totalsale += price;
                    sprintf(sendBuffer, "%s(이)가 나왔습니다. 잔돈은 %d입니다.", product, change, totalsale);
                }
                else {
                    int change = totalAmount;
                    sprintf(sendBuffer, "금액이 부족합니다. 남은 잔액은 %d입니다.", change);

                }
            }
            else {
                int change = totalAmount;
                sprintf(sendBuffer, "재고가 부족합니다. 남은 잔액은 %d입니다.", change);

            }
        }
    }
    else {
        strcpy(sendBuffer, "다시고르세요.");
    }


    // 클라이언트에게 응답보내기
    if (send(clientSocket, sendBuffer, strlen(sendBuffer), 0) == SOCKET_ERROR) {
        err_display("send() error");
    }

    
    bool inventoryShortage = false;
    std::string lowInventoryProducts;

    for (int i = 0; i < MAX_PRODUCTS; i++) {
        if (inventory[i] < 1) {
            inventoryShortage = true;

            switch (i) {
            case 0:
                lowInventoryProducts += "물";
                break;
            case 1:
                lowInventoryProducts += "커피";
                break;
            case 2:
                lowInventoryProducts += "이온 음료";
                break;
            case 3:
                lowInventoryProducts += "고급 커피";
                break;
            case 4:
                lowInventoryProducts += "탄산 음료";
                break;
            }

            lowInventoryProducts += ", ";
        }
    }
    // 재고부족 알림
    if (inventoryShortage) {
        std::string shortageMsg = "\n 재고가 부족합니다!: " + lowInventoryProducts;
        printf("%s\n", shortageMsg.c_str());
    }

    // 클라이언트 소켓 닫기
    closesocket(clientSocket);
    delete clientContext;
}

void StartServer() {
    
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        err_quit("WSAStartup() error");
    }

    
    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket == INVALID_SOCKET) {
        err_quit("socket() error");
    }

  
    SOCKADDR_IN serverAddr;
    ZeroMemory(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(SERVER_PORT);
    if (bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        err_quit("bind() error");
    }

    
    if (listen(listenSocket, 5) == SOCKET_ERROR) {
        err_quit("listen() error");
    }

    printf("%d포트 서버시작\n", SERVER_PORT);

    while (1) {
        
        SOCKADDR_IN clientAddr;
        int clientAddrSize = sizeof(clientAddr);
        SOCKET clientSocket = accept(listenSocket, (SOCKADDR*)&clientAddr, &clientAddrSize);
        if (clientSocket == INVALID_SOCKET) {
            err_display("accept() error");
            continue;
        }

        printf("클라이언트가 접속하였습니다.\n");
        // 클라이언트 요청을 다루기위한 스레드 생성
        ClientContext* clientContext = new ClientContext;
        clientContext->clientSocket = clientSocket;
        memcpy(&clientContext->clientAddr, &clientAddr, sizeof(clientAddr));
        std::thread clientThread(ClientRequest, clientContext);
        clientThread.detach();
    }

    // listenSocket 닫기
    closesocket(listenSocket);

   
    WSACleanup();
}

void Admin() {
    char choice;
    printf("관리자 창으로 진입하시겠습니까? (Y/N): ");
    scanf(" %c", &choice);
    getchar();  

    if (choice == 'Y' || choice == 'y') {
        printf("비밀번호: ");
        char password[20];
        fgets(password, sizeof(password), stdin);
        password[strcspn(password, "\n")] = '\0';  

        if (strcmp(password, adminPassword.c_str()) == 0) {
            printf("\n******************관리자창 진입******************\n");

            while (1) {
                printf("\n");
                printf("--------------------\n");
                printf("|관리자 메뉴       |\n");
                printf("--------------------\n");
                printf("|1. 비밀번호 변경  |\n");
                printf("|2. 재고 추가      |\n");
                printf("|3. 현재 재고 확인 |\n");
                printf("|4. 일별 매출      |\n");
                printf("|5. 월별 매출      |\n");
                printf("|6. 관리자 창나가기|\n");
                printf("--------------------\n");
                printf(">>> 선택: ");

                int choice;
                scanf("%d", &choice);
                getchar();  

                switch (choice) {
                case 1:
                {
                    printf("현재 비밀번호를 입력해주세요: ");
                    char currentPassword[20];
                    fgets(currentPassword, sizeof(currentPassword), stdin);
                    currentPassword[strcspn(currentPassword, "\n")] = '\0'; 
                    if (strcmp(currentPassword, adminPassword.c_str()) == 0) {
                        printf("새로운 비밀번호를 입력해주세요: ");
                        char newPassword[20];
                        fgets(newPassword, sizeof(newPassword), stdin);
                        newPassword[strcspn(newPassword, "\n")] = '\0';  

                       
                        bool isValidPassword = false;
                        bool hasDigit = false;
                        bool hasSpecialChar = false;

                        for (int i = 0; i < strlen(newPassword); i++) {
                            if (isdigit(newPassword[i])) {
                                hasDigit = true;
                            }
                            else if (ispunct(newPassword[i])) {
                                hasSpecialChar = true;
                            }
                        }

                        if (hasDigit && hasSpecialChar && strlen(newPassword) >= 8) {
                            adminPassword = newPassword;
                            printf("비밀번호가 변경되었습니다.\n");
                        }
                        else {
                            printf("유효하지 않은 비밀번호입니다. 비밀번호는 8자리 이상 특수문자 1개가 포함되어 있어야 합니다.\n");
                        }
                    }
                    else {
                        printf("비밀번호가 틀립니다.\n");
                    }
                    break;
                }
                case 2:
                {
                    int productChoice, amount;
                    printf("\n어떤 음료수를 채우실 건가요?: ");
                    scanf("%d", &productChoice);
                    getchar();  

                    if (productChoice >= 1 && productChoice <= MAX_PRODUCTS) {
                        printf("추가하고 싶은 개수를 입력하시오: ");
                        scanf("%d", &amount);
                        getchar();  

                        if (amount > 0) {
                            inventory[productChoice - 1] += amount;
                            printf("\n재고 충전 완료.\n");
                            printf("<현재 재고>\n");
                            printf("물: %d\n", inventory[0]);
                            printf("커피: %d\n", inventory[1]);
                            printf("이온 음료: %d\n", inventory[2]);
                            printf("고급 커피: %d\n", inventory[3]);
                            printf("탄산 음료: %d\n", inventory[4]);
                        }
                        else {
                            printf("Invalid amount.\n");
                        }
                    }
                    else {
                        printf("Invalid choice.\n");
                    }
                    break;
                }
                case 3:
                    printf("--------------------\n");
                    printf("\n현재 재고:\n");
                    printf("--------------------\n");
                    printf("|물: %d             |\n", inventory[0]);
                    printf("|커피: %d           |\n", inventory[1]);
                    printf("|이온 음료: %d      |\n", inventory[2]);
                    printf("|고급 커피: %d      |\n", inventory[3]);
                    printf("|탄산 음료: %d      |\n", inventory[4]);
                    printf("--------------------\n");
                    break;
                case 4:
                    printf("\n일별 매출:\n");
                    for (const auto& entry : dailySales) {
                        printf("%s: %d\n", entry.first.c_str(), entry.second);
                    }
                    break;
                case 5:
                    printf("\n월별 매출:\n");
                    for (const auto& entry : monthlySales) {
                        std::istringstream iss(entry.first);
                        int year, month;
                        char dash;
                        iss >> year >> dash >> month;
                        printf("%04d-%02d: %d\n", year, month, entry.second);
                    }
                    break;
                case 6:
                    printf("관리자 모드 종료.\n");
                    return;
                
                default:
                    printf("다시 고르세요.\n");
                    break;
                }

                bool inventory_shortage = false;
                std::string lowInventory_products;

                for (int i = 0; i < MAX_PRODUCTS; i++) {
                    if (inventory[i] < 1) {
                        inventory_shortage = true;

                        switch (i) {
                        case 0:
                            lowInventory_products += "물";
                            break;
                        case 1:
                            lowInventory_products += "커피";
                            break;
                        case 2:
                            lowInventory_products += "이온 음료";
                            break;
                        case 3:
                            lowInventory_products += "고급 커피";
                            break;
                        case 4:
                            lowInventory_products += "탄산 음료";
                            break;
                        }

                        lowInventory_products += ", ";
                    }
                }

                
                if (inventory_shortage)
                    lowInventory_products = lowInventory_products.substr(0, lowInventory_products.length() - 2);

                // 재고 부족할때 알림
                if (inventory_shortage) {
                    std::string shortageMsg = "재고가 부족합니다!: " + lowInventory_products;
                    printf("%s\n", shortageMsg.c_str());
                }

                // 파일 입출력
                std::ofstream inventoryFile("inventory.txt");
                if (inventoryFile.is_open()) {
                    inventoryFile << inventory[0] << "\n";
                    inventoryFile << inventory[1] << "\n";
                    inventoryFile << inventory[2] << "\n";
                    inventoryFile << inventory[3] << "\n";
                    inventoryFile << inventory[4] << "\n";
                    inventoryFile.close();
                }
                else {
                    printf("파일을 가져올수 없습니다.\n");
                }

                std::ofstream dailySalesFile("daily_sales.txt");
                if (dailySalesFile.is_open()) {
                    for (const auto& entry : dailySales) {
                        dailySalesFile << entry.first << "," << entry.second << "\n";
                    }
                    dailySalesFile.close();
                }
                else {
                    printf("파일을 가져올수 없습니다.\n");
                }

                std::ofstream monthlySalesFile("monthly_sales.2qtxt");
                if (monthlySalesFile.is_open()) {
                    for (const auto& entry : monthlySales) {
                        monthlySalesFile << entry.first << "," << entry.second << "\n";
                    }
                    monthlySalesFile.close();
                }
                else {
                    printf("파일을 가져올수 없습니다.\n");
                }
            }
        }
        else {
            printf("비밀번호가 틀립니다.\n");
        }
    }
}


int main() {
    // File I/O
    std::ifstream inventoryFile("inventory.txt");
    if (inventoryFile.is_open()) {
        for (int i = 0; i < MAX_PRODUCTS; i++) {
            inventoryFile >> inventory[i];
        }
        inventoryFile.close();
    }
    else {
        printf("파일을 가져올수 없습니다.\n");
    }

    std::ifstream dailySalesFile("daily_sales.txt");
    if (dailySalesFile.is_open()) {
        std::string line;
        while (std::getline(dailySalesFile, line)) {
            std::istringstream iss(line);
            std::string date;
            int amount;
            if (std::getline(iss, date, ',') && iss >> amount) {
                dailySales[date] = amount;
            }
        }
        dailySalesFile.close();
    }
    else {
        printf("파일을 가져올수 없습니다.\n");
    }

    std::ifstream monthlySalesFile("monthly_sales.txt");
    if (monthlySalesFile.is_open()) {
        std::string line;
        while (std::getline(monthlySalesFile, line)) {
            std::istringstream iss(line);
            std::string month;
            int amount;
            if (std::getline(iss, month, ',') && iss >> amount) {
                monthlySales[month] = amount;
            }
        }
        monthlySalesFile.close();
    }
    else {
        printf("파일을 가져올수 없습니다.\n");
    }

    std::thread serverThread(StartServer);
    serverThread.detach();

    while (1) {
        Admin();
    }

    return 0;
}