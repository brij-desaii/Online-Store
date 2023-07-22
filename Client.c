#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include "structs.h"

char pid[10] = "ProductID";
char pname[12] = "ProductName";
char q[19] = "Quantity available";
char p[6] = "Price";

void printProduct(struct product p)
{
    if (p.id != -1 && p.qty > 0)
    {
        printf("| %9d | %-11s | %18d | %5d |\n", p.id, p.name, p.qty, p.price);
    }
}

void printInventory(int sd)
{
    printf("Fetching data\n");
    printf("| %s | %s | %s | %s |\n", pid, pname, q, p);
    while (1)
    {
        struct product p;
        read(sd, &p, sizeof(struct product));
        if (p.id != -1)
        {
            printProduct(p);
        }
        else
        {
            break;
        }
    }
    printf("\n");
}

void generateReceipt(int total, struct cart c, int sd)
{

    write(sd, &total, sizeof(int));
    write(sd, &c, sizeof(struct cart));
}

int main()
{
    printf("Connecting to server...\n");
    int sd = socket(AF_INET, SOCK_STREAM, 0);

    if (sd == -1)
    {
        perror("Error: ");
        return -1;
    }

    struct sockaddr_in serv;

    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = INADDR_ANY;
    serv.sin_port = htons(6767);

    if (connect(sd, (struct sockaddr *)&serv, sizeof(serv)) == -1)
    {
        perror("Error: ");
        return -1;
    }

    printf("Connected.\n");
    printf("Enter 1 to proceed as user, or 2 to proceed as admin\n");
    int user;
    scanf("%d", &user);
    write(sd, &user, sizeof(user));

    if (user == 1)
    {
        while (1)
        {
            char choice;

            printf("====================\n");
            printf("|   MENU TO CHOOSE  |\n");
            printf("====================\n");
            printf("| a. Register       |\n");
            printf("| b. View Products  |\n");
            printf("| c. View Cart      |\n");
            printf("| d. Add to Cart    |\n");
            printf("| e. Edit Cart Item |\n");
            printf("| f. Payment        |\n");
            printf("| g. Exit           |\n");
            printf("====================\n");
            printf("Please enter your choice: ");

            scanf("%c", &choice);
            scanf("%c", &choice);

            write(sd, &choice, sizeof(char));

            if (choice == 'a')
            {
                int id;
                read(sd, &id, sizeof(int));
                printf("Your new customer id : %d\n", id);
            }
            else if (choice == 'b')
            {
                printInventory(sd);
            }
            else if (choice == 'c')
            {
                int customer_id;
                printf("Enter customer id\n");
                scanf("%d", &customer_id);

                write(sd, &customer_id, sizeof(int));

                struct cart c;
                read(sd, &c, sizeof(struct cart));

                if (c.customer_id != -1)
                {
                    printf("\n");
                    printf("| %s | %s | %s | %s |\n", pid, pname, q, p);
                    for (int i = 0; i < PROD_LIMIT; i++)
                    {
                        printProduct(c.products[i]);
                    }
                }
                else
                {
                    printf("Please enter correct customer ID\n");
                }
            }

            else if (choice == 'd')
            {
                int customer_id, resp;
                printf("Enter customer id\n");
                scanf("%d", &customer_id);

                write(sd, &customer_id, sizeof(int));

                read(sd, &resp, sizeof(int));
                if (resp == -1)
                {
                    printf("Please enter correct customer ID\n");
                    continue;
                }

                int prod_id;
                int qty = -1;

                printf("Enter product id: \n");
                scanf("%d", &prod_id);

                while (qty <= 0)
                {
                    printf("Enter quantity\n");
                    scanf("%d", &qty);

                    if (qty <= 0)
                    {
                        printf("Quantity needs to be a positive integer, please try again\n");
                    }
                    else
                    {
                        break;
                    }
                }

                struct product p;
                p.id = prod_id;
                p.qty = qty;

                char response[100];

                write(sd, &p, sizeof(struct product));
                read(sd, response, sizeof(response));
                printf("%s", response);
            }

            else if (choice == 'e')
            {
                int customer_id, resp;
                printf("Enter customer ID: ");
                scanf("%d", &customer_id);

                write(sd, &customer_id, sizeof(int));
                read(sd, &resp, sizeof(int));

                if (resp == -1)
                {
                    printf("Invalid customer id\n");
                    continue;
                }

                struct product p;

                printf("Enter product ID: ");
                scanf("%d", &p.id);
                printf("Enter quantity: ");
                scanf("%d", &p.qty);

                write(sd, &p, sizeof(struct product));

                char response[100];

                read(sd, response, sizeof(response));
                printf("%s", response);
            }

            else if (choice == 'f')
            {
                int customer_id, resp;
                printf("Enter customer ID: ");
                scanf("%d", &customer_id);

                write(sd, &customer_id, sizeof(int));

                read(sd, &resp, sizeof(int));

                if (resp == -1)
                {
                    printf("Customer with given ID doesn't exist\n");
                    continue;
                }

                struct cart c;
                read(sd, &c, sizeof(struct cart));

                int ordered, available, price;
                for (int i = 0; i < PROD_LIMIT; i++)
                {
                    if (c.products[i].id != -1)
                    {
                        read(sd, &ordered, sizeof(int));
                        read(sd, &available, sizeof(int));
                        read(sd, &price, sizeof(int));
                        printf("Product id- %d\n", c.products[i].id);
                        printf("Ordered - %d; Available - %d; Price - %d\n", ordered, available, price);
                        c.products[i].qty = available;
                        c.products[i].price = price;
                    }
                }

                int total = 0;
                for (int i = 0; i < PROD_LIMIT; i++)
                {
                    if (c.products[i].id != -1)
                    {
                        total += c.products[i].qty * c.products[i].price;
                    }
                }

                printf("Total in your cart\n");
                printf("%d\n", total);

                int payment = -1;
                while (payment != total)
                {
                    printf("Please enter the amount to pay\n");
                    scanf("%d", &payment);

                    if (payment != total)
                    {
                        printf("Wrong amount entered, enter again\n");
                    }
                    else
                    {
                        break;
                    }
                }

                char ch = 'y';
                printf("Payment recorded, order placed\n");
                write(sd, &ch, sizeof(char));
                read(sd, &ch, sizeof(char));

                generateReceipt(total, c, sd);
            }

            else if (choice == 'g')
            {
                break;
            }
            else
            {
                printf("Invalid choice, try again\n");
            }
        }
    }
    else if (user == 2)
    {

        while (1)
        {
            char choice;

            printf("========================\n");
            printf("|    MENU TO CHOOSE    |\n");
            printf("========================\n");
            printf("| a. Add Product       |\n");
            printf("| b. Delete Product    |\n");
            printf("| c. Update Price      |\n");
            printf("| d. Update Quantity   |\n");
            printf("| e. View Inventory    |\n");
            printf("| f. Exit Program      |\n");
            printf("========================\n");
            printf("Please enter your choice: ");

            scanf("%c", &choice);
            scanf("%c", &choice);

            write(sd, &choice, sizeof(choice));

            if (choice == 'a')
            {
                char name[50], response[100];
                struct product p;

                printf("Enter product name\n");
                scanf("%s", name);
                strcpy(p.name, name);
                printf("Enter product id\n");
                scanf("%d", &p.id);
                printf("Enter price\n");
                scanf("%d", &p.price);
                printf("Enter quantity\n");
                scanf("%d", &p.qty);

                write(sd, &p, sizeof(struct product));

                int n = read(sd, response, sizeof(response));
                response[n] = '\0';

                printf("%s", response);
            }

            else if (choice == 'b')
            {
                int prod_id;
                char response[100];

                printf("Enter product id: \n");
                scanf("%d", &prod_id);

                write(sd, &prod_id, sizeof(int));

                read(sd, response, sizeof(response));
                printf("%s\n", response);
            }

            else if (choice == 'c')
            {
                struct product p;
                char response[100];

                printf("Enter product id: \n");
                scanf("%d", &p.id);
                printf("Enter price: \n");
                scanf("%d", &p.price);

                write(sd, &p, sizeof(struct product));

                read(sd, response, sizeof(response));
                printf("%s\n", response);
            }

            else if (choice == 'd')
            {
                struct product p;
                char response[100];

                printf("Enter product id: \n");
                scanf("%d", &p.id);
                printf("Enter quantity: \n");
                scanf("%d", &p.qty);

                write(sd, &p, sizeof(struct product));

                read(sd, response, sizeof(response));
                printf("%s\n", response);
            }

            else if (choice == 'e')
            {
                printInventory(sd);
            }

            else if (choice == 'f')
            {
                break;
            }

            else
            {
                printf("Invalid choice, try again\n");
            }
        }
    }

    printf("Exiting program\n");
    close(sd);
    return 0;
}