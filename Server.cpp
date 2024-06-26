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

// ������ ��й�ȣ
std::string adminPassword = "1111";

void ClientRequest(ClientContext* clientContext) {
    SOCKET clientSocket = clientContext->clientSocket;
    SOCKADDR_IN clientAddr = clientContext->clientAddr;
    char recvBuffer[2000];
    char sendBuffer[2000];

    // Ŭ���̾�Ʈ�κ��� ������ �ޱ�
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
        // �޴��� ��� �����ֱ�
        sprintf(sendBuffer, "Inventory: �� (%d), Ŀ�� (%d), �̿� ���� (%d), ��� Ŀ�� (%d), ź�� ���� (%d)",
            inventory[0], inventory[1], inventory[2], inventory[3], inventory[4]);
    }
    else if (choice >= 1 && choice <= MAX_PRODUCTS) {
        int price = 0;
        const char* product = nullptr;

        switch (choice) {
        case 1:
            price = 450;
            product = "��";
            break;
        case 2:
            price = 500;
            product = "Ŀ��";
            break;
        case 3:
            price = 550;
            product = "�̿� ����";
            break;
        case 4:
            price = 700;
            product = "��� Ŀ��";
            break;
        case 5:
            price = 750;
            product = "ź�� ����";
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
                    sprintf(sendBuffer, "%s(��)�� ���Խ��ϴ�. �ܵ��� %d�Դϴ�.", product, change, totalsale);
                }
                else {
                    int change = totalAmount;
                    sprintf(sendBuffer, "�ݾ��� �����մϴ�. ���� �ܾ��� %d�Դϴ�.", change);

                }
            }
            else {
                int change = totalAmount;
                sprintf(sendBuffer, "��� �����մϴ�. ���� �ܾ��� %d�Դϴ�.", change);

            }
        }
    }
    else {
        strcpy(sendBuffer, "�ٽð�����.");
    }


    // Ŭ���̾�Ʈ���� ���亸����
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
                lowInventoryProducts += "��";
                break;
            case 1:
                lowInventoryProducts += "Ŀ��";
                break;
            case 2:
                lowInventoryProducts += "�̿� ����";
                break;
            case 3:
                lowInventoryProducts += "��� Ŀ��";
                break;
            case 4:
                lowInventoryProducts += "ź�� ����";
                break;
            }

            lowInventoryProducts += ", ";
        }
    }
    // ������ �˸�
    if (inventoryShortage) {
        std::string shortageMsg = "\n ��� �����մϴ�!: " + lowInventoryProducts;
        printf("%s\n", shortageMsg.c_str());
    }

    // Ŭ���̾�Ʈ ���� �ݱ�
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

    printf("%d��Ʈ ��������\n", SERVER_PORT);

    while (1) {
        
        SOCKADDR_IN clientAddr;
        int clientAddrSize = sizeof(clientAddr);
        SOCKET clientSocket = accept(listenSocket, (SOCKADDR*)&clientAddr, &clientAddrSize);
        if (clientSocket == INVALID_SOCKET) {
            err_display("accept() error");
            continue;
        }

        printf("Ŭ���̾�Ʈ�� �����Ͽ����ϴ�.\n");
        // Ŭ���̾�Ʈ ��û�� �ٷ������ ������ ����
        ClientContext* clientContext = new ClientContext;
        clientContext->clientSocket = clientSocket;
        memcpy(&clientContext->clientAddr, &clientAddr, sizeof(clientAddr));
        std::thread clientThread(ClientRequest, clientContext);
        clientThread.detach();
    }

    // listenSocket �ݱ�
    closesocket(listenSocket);

   
    WSACleanup();
}

void Admin() {
    char choice;
    printf("������ â���� �����Ͻðڽ��ϱ�? (Y/N): ");
    scanf(" %c", &choice);
    getchar();  

    if (choice == 'Y' || choice == 'y') {
        printf("��й�ȣ: ");
        char password[20];
        fgets(password, sizeof(password), stdin);
        password[strcspn(password, "\n")] = '\0';  

        if (strcmp(password, adminPassword.c_str()) == 0) {
            printf("\n******************������â ����******************\n");

            while (1) {
                printf("\n");
                printf("--------------------\n");
                printf("|������ �޴�       |\n");
                printf("--------------------\n");
                printf("|1. ��й�ȣ ����  |\n");
                printf("|2. ��� �߰�      |\n");
                printf("|3. ���� ��� Ȯ�� |\n");
                printf("|4. �Ϻ� ����      |\n");
                printf("|5. ���� ����      |\n");
                printf("|6. ������ â������|\n");
                printf("--------------------\n");
                printf(">>> ����: ");

                int choice;
                scanf("%d", &choice);
                getchar();  

                switch (choice) {
                case 1:
                {
                    printf("���� ��й�ȣ�� �Է����ּ���: ");
                    char currentPassword[20];
                    fgets(currentPassword, sizeof(currentPassword), stdin);
                    currentPassword[strcspn(currentPassword, "\n")] = '\0'; 
                    if (strcmp(currentPassword, adminPassword.c_str()) == 0) {
                        printf("���ο� ��й�ȣ�� �Է����ּ���: ");
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
                            printf("��й�ȣ�� ����Ǿ����ϴ�.\n");
                        }
                        else {
                            printf("��ȿ���� ���� ��й�ȣ�Դϴ�. ��й�ȣ�� 8�ڸ� �̻� Ư������ 1���� ���ԵǾ� �־�� �մϴ�.\n");
                        }
                    }
                    else {
                        printf("��й�ȣ�� Ʋ���ϴ�.\n");
                    }
                    break;
                }
                case 2:
                {
                    int productChoice, amount;
                    printf("\n� ������� ä��� �ǰ���?: ");
                    scanf("%d", &productChoice);
                    getchar();  

                    if (productChoice >= 1 && productChoice <= MAX_PRODUCTS) {
                        printf("�߰��ϰ� ���� ������ �Է��Ͻÿ�: ");
                        scanf("%d", &amount);
                        getchar();  

                        if (amount > 0) {
                            inventory[productChoice - 1] += amount;
                            printf("\n��� ���� �Ϸ�.\n");
                            printf("<���� ���>\n");
                            printf("��: %d\n", inventory[0]);
                            printf("Ŀ��: %d\n", inventory[1]);
                            printf("�̿� ����: %d\n", inventory[2]);
                            printf("��� Ŀ��: %d\n", inventory[3]);
                            printf("ź�� ����: %d\n", inventory[4]);
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
                    printf("\n���� ���:\n");
                    printf("--------------------\n");
                    printf("|��: %d             |\n", inventory[0]);
                    printf("|Ŀ��: %d           |\n", inventory[1]);
                    printf("|�̿� ����: %d      |\n", inventory[2]);
                    printf("|��� Ŀ��: %d      |\n", inventory[3]);
                    printf("|ź�� ����: %d      |\n", inventory[4]);
                    printf("--------------------\n");
                    break;
                case 4:
                    printf("\n�Ϻ� ����:\n");
                    for (const auto& entry : dailySales) {
                        printf("%s: %d\n", entry.first.c_str(), entry.second);
                    }
                    break;
                case 5:
                    printf("\n���� ����:\n");
                    for (const auto& entry : monthlySales) {
                        std::istringstream iss(entry.first);
                        int year, month;
                        char dash;
                        iss >> year >> dash >> month;
                        printf("%04d-%02d: %d\n", year, month, entry.second);
                    }
                    break;
                case 6:
                    printf("������ ��� ����.\n");
                    return;
                
                default:
                    printf("�ٽ� ������.\n");
                    break;
                }

                bool inventory_shortage = false;
                std::string lowInventory_products;

                for (int i = 0; i < MAX_PRODUCTS; i++) {
                    if (inventory[i] < 1) {
                        inventory_shortage = true;

                        switch (i) {
                        case 0:
                            lowInventory_products += "��";
                            break;
                        case 1:
                            lowInventory_products += "Ŀ��";
                            break;
                        case 2:
                            lowInventory_products += "�̿� ����";
                            break;
                        case 3:
                            lowInventory_products += "��� Ŀ��";
                            break;
                        case 4:
                            lowInventory_products += "ź�� ����";
                            break;
                        }

                        lowInventory_products += ", ";
                    }
                }

                
                if (inventory_shortage)
                    lowInventory_products = lowInventory_products.substr(0, lowInventory_products.length() - 2);

                // ��� �����Ҷ� �˸�
                if (inventory_shortage) {
                    std::string shortageMsg = "��� �����մϴ�!: " + lowInventory_products;
                    printf("%s\n", shortageMsg.c_str());
                }

                // ���� �����
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
                    printf("������ �����ü� �����ϴ�.\n");
                }

                std::ofstream dailySalesFile("daily_sales.txt");
                if (dailySalesFile.is_open()) {
                    for (const auto& entry : dailySales) {
                        dailySalesFile << entry.first << "," << entry.second << "\n";
                    }
                    dailySalesFile.close();
                }
                else {
                    printf("������ �����ü� �����ϴ�.\n");
                }

                std::ofstream monthlySalesFile("monthly_sales.2qtxt");
                if (monthlySalesFile.is_open()) {
                    for (const auto& entry : monthlySales) {
                        monthlySalesFile << entry.first << "," << entry.second << "\n";
                    }
                    monthlySalesFile.close();
                }
                else {
                    printf("������ �����ü� �����ϴ�.\n");
                }
            }
        }
        else {
            printf("��й�ȣ�� Ʋ���ϴ�.\n");
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
        printf("������ �����ü� �����ϴ�.\n");
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
        printf("������ �����ü� �����ϴ�.\n");
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
        printf("������ �����ü� �����ϴ�.\n");
    }

    std::thread serverThread(StartServer);
    serverThread.detach();

    while (1) {
        Admin();
    }

    return 0;
}