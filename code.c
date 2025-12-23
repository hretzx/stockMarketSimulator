#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#define N 26  // Hash size

typedef struct Trade {
    char stock[10];
    int price;
    int quantity;
    time_t timestamp;
    struct Trade *next;
} Trade;

typedef struct TradeList {
    Trade *start;
} TradeList;

typedef struct AVLNode {
    char stock[10];
    int price;
    struct AVLNode *left, *right;
    int height;
} AVLNode;

typedef struct Order {
    char stock[10];
    int price;
    int quantity;
    time_t timestamp;
    struct Order *next;
} Order;

typedef struct OrderList {
    Order *start;
} OrderList;

// Now, we have arrays of wrapper structs for buy/sell orders
OrderList buyOrders[N];
OrderList sellOrders[N];
TradeList tradeHistory = {NULL};
AVLNode* stockPrices = NULL;

int hash(char* symbol) {
    int hashValue = 0;
    for (int i = 0; symbol[i] != '\0'; i++) {
        hashValue += symbol[i];
    }
    return hashValue % N;
}

// AVL helper functions (unchanged)
int height(AVLNode* node) {
    return node ? node->height : 0;
}

int max(int a, int b) {
    return (a > b) ? a : b;
}

AVLNode* createNode(char* stock, int price) {
    AVLNode* newNode = (AVLNode*)malloc(sizeof(AVLNode));
    strcpy(newNode->stock, stock);
    newNode->price = price;
    newNode->left = newNode->right = NULL;
    newNode->height = 1;
    return newNode;
}

AVLNode* rightRotate(AVLNode* y) {
    AVLNode* x = y->left;
    AVLNode* T2 = x->right;

    x->right = y;
    y->left = T2;

    y->height = max(height(y->left), height(y->right)) + 1;
    x->height = max(height(x->left), height(x->right)) + 1;

    return x;
}

AVLNode* leftRotate(AVLNode* x) {
    AVLNode* y = x->right;
    AVLNode* T2 = y->left;

    y->left = x;
    x->right = T2;

    x->height = max(height(x->left), height(x->right)) + 1;
    y->height = max(height(y->left), height(y->right)) + 1;

    return y;
}

int getBalance(AVLNode* node) {
    return node ? height(node->left) - height(node->right) : 0;
}

AVLNode* insertAVL(AVLNode* node, char* stock, int price) {
    if (!node)
        return createNode(stock, price);

    int cmp = strcmp(stock, node->stock);
    if (cmp < 0)
        node->left = insertAVL(node->left, stock, price);
    else if (cmp > 0)
        node->right = insertAVL(node->right, stock, price);
    else {
        node->price = price;
        return node;
    }

    node->height = max(height(node->left), height(node->right)) + 1;
    int balance = getBalance(node);

    if (balance > 1 && strcmp(stock, node->left->stock) < 0)
        return rightRotate(node);
    if (balance < -1 && strcmp(stock, node->right->stock) > 0)
        return leftRotate(node);
    if (balance > 1 && strcmp(stock, node->left->stock) > 0) {
        node->left = leftRotate(node->left);
        return rightRotate(node);
    }
    if (balance < -1 && strcmp(stock, node->right->stock) < 0) {
        node->right = rightRotate(node->right);
        return leftRotate(node);
    }

    return node;
}

void inOrderPrices(AVLNode* root) {
    if (!root) return;
    inOrderPrices(root->left);
    printf("Stock: %s | Price: %d\n", root->stock, root->price);
    inOrderPrices(root->right);
}

// Insert order into wrapper list
void insertOrder(OrderList *list, char* stock, int price, int quantity) {
    Order* newOrder = (Order*)malloc(sizeof(Order));
    strcpy(newOrder->stock, stock);
    newOrder->price = price;
    newOrder->quantity = quantity;
    newOrder->timestamp = time(NULL);
    newOrder->next = NULL;

    Order* curr = list->start;
    Order* prev = NULL;

    while (curr) {
        int cmp = (price <= curr->price);
        if (!cmp) break;
        prev = curr;
        curr = curr->next;
    }

    if (!prev) {
        newOrder->next = list->start;
        list->start = newOrder;
    } else {
        newOrder->next = curr;
        prev->next = newOrder;
    }
}

// Add trade to trade history wrapper
void addTrade(char* stock, int price, int quantity) {
    Trade* newTrade = (Trade*)malloc(sizeof(Trade));
    strcpy(newTrade->stock, stock);
    newTrade->price = price;
    newTrade->quantity = quantity;
    newTrade->timestamp = time(NULL);
    newTrade->next = tradeHistory.start;
    tradeHistory.start = newTrade;
}

// Matching using wrapper lists
void matchOrders(char* stock, char type[5], int price, int quantity) {
    int index = hash(stock);
    OrderList *opposingList = (strcmp(type, "buy") == 0) ? &sellOrders[index] : &buyOrders[index];
    Order* curr = opposingList->start;
    Order* prev = NULL;

    int tradeOccurred = 0;
    int lastTradePrice = 0;

    while (curr && quantity > 0) {
        if (strcmp(curr->stock, stock) == 0 &&
            ((strcmp(type, "buy") == 0 && price >= curr->price) ||
             (strcmp(type, "sell") == 0 && price <= curr->price))) {

            int tradedQty = (quantity < curr->quantity) ? quantity : curr->quantity;
            int tradePrice = (strcmp(type, "buy") == 0) ? curr->price : price;
            addTrade(stock, tradePrice, tradedQty);
            lastTradePrice = tradePrice;
            tradeOccurred = 1;

            curr->quantity -= tradedQty;
            quantity -= tradedQty;

            if (curr->quantity == 0) {
                if (prev) prev->next = curr->next;
                else opposingList->start = curr->next;
                free(curr);
                curr = (prev) ? prev->next : opposingList->start;
            } else {
                break;
            }
        } else {
            prev = curr;
            curr = curr->next;
        }
    }

    if (quantity > 0) {
        if (strcmp(type, "buy") == 0)
            insertOrder(&buyOrders[index], stock, price, quantity);
        else
            insertOrder(&sellOrders[index], stock, price, quantity);
    }

    int updatedPrice = tradeOccurred ? lastTradePrice : price;
    stockPrices = insertAVL(stockPrices, stock, updatedPrice);
}

void displayTradeHistory() {
    Trade* curr = tradeHistory.start;
    if (!curr) {
        printf("No trades have occurred yet.\n");
        return;
    }

    while (curr) {
        printf("Stock: %s | Price: %d | Quantity: %d | Time: %s",
               curr->stock, curr->price, curr->quantity, ctime(&(curr->timestamp)));
        curr = curr->next;
    }
}

void printPendingOrders() {
    int hasPendingBuyOrders = 0;
    printf("\n\t\t\t\tPending Buy Orders\n");
    for (int i = 0; i < N; i++) {
        Order* curr = buyOrders[i].start;
        while (curr) {
            printf("Stock: %s | Price: %d | Quantity: %d | Time: %s",
                   curr->stock, curr->price, curr->quantity, ctime(&(curr->timestamp)));
            curr = curr->next;
            hasPendingBuyOrders = 1;
        }
    }
    if (!hasPendingBuyOrders) {
        printf("No pending buy orders.\n");
    }

    int hasPendingSellOrders = 0;
    printf("\n\t\t\t\tPending Sell Orders\n");
    for (int i = 0; i < N; i++) {
        Order* curr = sellOrders[i].start;
        while (curr) {
            printf("Stock: %s | Price: %d | Quantity: %d | Time: %s",
                   curr->stock, curr->price, curr->quantity, ctime(&(curr->timestamp)));
            curr = curr->next;
            hasPendingSellOrders = 1;
        }
    }
    if (!hasPendingSellOrders) {
        printf("No pending sell orders.\n");
    }
}

void displayCurrentPrices() {
    if (!stockPrices)
        printf("No stock prices available.\n");
    else {
        printf("\nCurrent Stock Prices:\n");
        inOrderPrices(stockPrices);
    }
}

int main() {
    int choice;
    printf("\t\t\t\tStock Market Simulator!!\n");
    while (1) {
        printf("\n1. Buy\n2. Sell\n3. Trade History\n4. View Current Prices\n5. View Pending Orders\n6. Exit\nEnter choice: ");
        scanf("%d", &choice);
        if (choice == 6) {
            break;
        }
        switch (choice) {
            case 1:
            case 2: {
                char stock[10];
                int price, quantity;
                printf("Enter Stock Symbol: ");
                scanf("%s", stock);
                printf("Enter Price: ");
                if (scanf("%d", &price) != 1 || price <= 0) {
                    printf("Invalid input. Price must be a positive number.\n");
                    while (getchar() != '\n');
                    break;
                }
                printf("Enter Quantity: ");
                if (scanf("%d", &quantity) != 1 || quantity <= 0) {
                    printf("Invalid input. Quantity must be a positive number.\n");
                    while (getchar() != '\n');
                    break;
                }
                matchOrders(stock, (choice == 1) ? "buy" : "sell", price, quantity);
                break;
            }

            case 3:
                displayTradeHistory();
                break;

            case 4:
                displayCurrentPrices();
                break;

            case 5:
                printPendingOrders();
                break;

            default:
                printf("Invalid choice.\n");
                break;
        }
    }
}
