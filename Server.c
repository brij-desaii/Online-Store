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

void unlock(int fd, struct flock lock)
{
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLKW, &lock);
}

void inventoryReadLock(int fd_inventory, struct flock lock)
{
    lock.l_len = 0;
    lock.l_type = F_RDLCK;
    lock.l_start = 0;
    lock.l_whence = SEEK_SET;
    fcntl(fd_inventory, F_SETLKW, &lock);
}

void inventoryWriteLock(int fd_inventory, struct flock lock)
{
    lseek(fd_inventory, (-1) * sizeof(struct product), SEEK_CUR);
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_CUR;
    lock.l_start = 0;
    lock.l_len = sizeof(struct product);

    fcntl(fd_inventory, F_SETLKW, &lock);
}

void cartRecordLock(int fd_cart, struct flock lock_cart, int offset, int ch)
{
    lock_cart.l_whence = SEEK_SET;
    lock_cart.l_len = sizeof(struct cart);
    lock_cart.l_start = offset;
    if (ch == 1)
    {
        lock_cart.l_type = F_RDLCK;
    }
    else
    {
        lock_cart.l_type = F_WRLCK;
    }
    fcntl(fd_cart, F_SETLKW, &lock_cart);
    lseek(fd_cart, offset, SEEK_SET);
}

void readLockCust(int fd_customers, struct flock lock_cust)
{

    lock_cust.l_len = 0;
    lock_cust.l_type = F_RDLCK;
    lock_cust.l_start = 0;
    lock_cust.l_whence = SEEK_SET;
    fcntl(fd_customers, F_SETLKW, &lock_cust);

    return;
}

int getOffset(int cust_id, int fd_customers)
{
    if (cust_id < 0)
    {
        return -1;
    }
    struct flock lock_cust;
    readLockCust(fd_customers, lock_cust);
    struct cart_index id;

    while (read(fd_customers, &id, sizeof(struct cart_index)))
    {
        if (id.customer_id == cust_id)
        {
            unlock(fd_customers, lock_cust);
            return id.offset;
        }
    }
    unlock(fd_customers, lock_cust);
    return -1;
}

void registerCustomer(int fd_cart, int fd_customers, int nsd)
{
    struct flock lock;
    readLockCust(fd_customers, lock);

    int new_id = -1;
    struct cart_index id;
    while (read(fd_customers, &id, sizeof(struct cart_index)))
    {
        if (id.customer_id > new_id)
        {
            new_id = id.customer_id;
        }
    }

    new_id++;

    id.customer_id = new_id;
    id.offset = lseek(fd_cart, 0, SEEK_END);
    lseek(fd_customers, 0, SEEK_END);
    write(fd_customers, &id, sizeof(struct cart_index));

    struct cart c;
    c.customer_id = new_id;
    for (int i = 0; i < PROD_LIMIT; i++)
    {
        c.products[i].id = -1;
        strcpy(c.products[i].name, "");
        c.products[i].qty = -1;
        c.products[i].price = -1;
    }

    write(fd_cart, &c, sizeof(struct cart));
    unlock(fd_customers, lock);
    write(nsd, &new_id, sizeof(int));
}

void listInventory(int fd_inventory, int nsd)
{

    struct flock lock;
    inventoryReadLock(fd_inventory, lock);

    struct product p;
    while (read(fd_inventory, &p, sizeof(struct product)))
    {
        if (p.id != -1)
        {
            write(nsd, &p, sizeof(struct product));
        }
    }

    p.id = -1;
    write(nsd, &p, sizeof(struct product));
    unlock(fd_inventory, lock);
}

void addProducts(int fd_inventory, int nsd, int fd_admin)
{

    char name[50];
    char log[100];
    int id, qty, price;
    struct product p1, p;
    int flg = 0;

    read(nsd, &p1, sizeof(struct product));

    strcpy(name, p1.name);
    id = p1.id;
    qty = p1.qty;
    price = p1.price;

    struct flock lock;
    inventoryReadLock(fd_inventory, lock);

    while (read(fd_inventory, &p, sizeof(struct product)))
    {

        if (p.id == id && p.qty > 0)
        {
            write(nsd, "Product with given ID already exists\n", sizeof("Product with given ID already exists\n"));

            sprintf(log, "Couldn't add product with product id %d as it already exists\n", id);
            write(fd_admin, log, strlen(log));

            unlock(fd_inventory, lock);
            flg = 1;
            return;
        }
    }

    if (!flg)
    {

        lseek(fd_inventory, 0, SEEK_END);

        p.id = id;
        strcpy(p.name, name);
        p.price = price;
        p.qty = qty;

        write(fd_inventory, &p, sizeof(struct product));
        write(nsd, "Product added successfully\n", sizeof("Product added succesfully\n"));
        sprintf(log, "New product with product id %d added to inventory\n", id);
        write(fd_admin, log, strlen(log));
        unlock(fd_inventory, lock);
    }
}

void updateProduct(int fd_inventory, int nsd, int opt, int fd_admin)
{
    int id;
    char log[100];
    int val = -1;

    struct product p1;
    read(nsd, &p1, sizeof(struct product));
    id = p1.id;

    if (opt != 1)
    {
        val = p1.qty;
    }
    else
    {
        val = p1.price;
    }

    struct flock lock;
    inventoryReadLock(fd_inventory, lock);

    int flg = 0;

    struct product p;
    while (read(fd_inventory, &p, sizeof(struct product)))
    {
        if (p.id == id)
        {

            unlock(fd_inventory, lock);
            inventoryWriteLock(fd_inventory, lock);
            int old;
            if (opt == 1)
            {
                old = p.price;
                p.price = val;
            }
            else
            {
                old = p.qty;
                p.qty = val;
            }

            write(fd_inventory, &p, sizeof(struct product));
            if (opt == 1)
            {
                write(nsd, "Price updated successfully", sizeof("Price updated successfully"));
                sprintf(log, "Price of product with product id %d updated from %d to %d \n", id, old, val);
                write(fd_admin, log, strlen(log));
            }
            else
            {
                sprintf(log, "Quantity of product with product id %d updated from %d to %d \n", id, old, val);
                write(fd_admin, log, strlen(log));
                write(nsd, "Quantity updated successfully", sizeof("Quantity updated successfully"));
            }

            unlock(fd_inventory, lock);
            flg = 1;
            break;
        }
    }

    if (!flg)
    {
        write(nsd, "Product id invalid", sizeof("Product id invalid"));
        unlock(fd_inventory, lock);
    }
}

void deleteProduct(int fd_inventory, int nsd, int id, int fd_admin)
{

    struct flock lock;
    inventoryReadLock(fd_inventory, lock);
    char log[100];

    struct product p;
    int flg = 0;
    while (read(fd_inventory, &p, sizeof(struct product)))
    {
        if (p.id == id)
        {

            unlock(fd_inventory, lock);

            inventoryWriteLock(fd_inventory, lock);

            p.id = -1;
            p.price = -1;
            p.qty = -1;
            strcpy(p.name, "");

            write(fd_inventory, &p, sizeof(struct product));

            sprintf(log, "Product with product id %d deleted succesfully\n", id);
            write(fd_admin, log, strlen(log));

            write(nsd, "Deleted product successfully", sizeof("Deleted product successfully"));

            unlock(fd_inventory, lock);
            flg = 1;
            return;
        }
    }

    if (!flg)
    {
        sprintf(log, "Couldn't delete product with product id %d because it does not exit\n", id);
        write(fd_admin, log, strlen(log));
        write(nsd, "Product id invalid", sizeof("Product id invalid"));
        unlock(fd_inventory, lock);
    }
}

void viewCart(int fd_cart, int nsd, int fd_customers)
{
    int cust_id = -1;
    read(nsd, &cust_id, sizeof(int));

    int offset = getOffset(cust_id, fd_customers);
    struct cart c;

    if (offset == -1)
    {

        struct cart c;
        c.customer_id = -1;
        write(nsd, &c, sizeof(struct cart));
    }
    else
    {
        struct cart c;
        struct flock lock_cart;

        cartRecordLock(fd_cart, lock_cart, offset, 1);
        read(fd_cart, &c, sizeof(struct cart));
        write(nsd, &c, sizeof(struct cart));
        unlock(fd_cart, lock_cart);
    }
}

void editCartItem(int fd, int fd_cart, int fd_customers, int nsd)
{

    int cust_id = -1;
    read(nsd, &cust_id, sizeof(int));

    int offset = getOffset(cust_id, fd_customers);

    write(nsd, &offset, sizeof(int));
    if (offset == -1)
    {
        return;
    }

    struct flock lock_cart;
    cartRecordLock(fd_cart, lock_cart, offset, 1);
    struct cart c;
    read(fd_cart, &c, sizeof(struct cart));

    int pid, qty;
    struct product p;
    read(nsd, &p, sizeof(struct product));

    pid = p.id;
    qty = p.qty;

    int flg = 0;
    int i;
    for (i = 0; i < PROD_LIMIT; i++)
    {
        if (c.products[i].id == pid)
        {

            struct flock lock_prod;
            inventoryReadLock(fd, lock_prod);

            struct product p1;
            while (read(fd, &p1, sizeof(struct product)))
            {
                if (p1.id == pid && p1.qty > 0)
                {
                    if (p1.qty >= qty)
                    {
                        flg = 1;
                        break;
                    }
                    else
                    {
                        flg = 0;
                        break;
                    }
                }
            }

            unlock(fd, lock_prod);
            break;
        }
    }
    unlock(fd_cart, lock_cart);

    if (!flg)
    {
        write(nsd, "Product id not in the order or out of stock\n", sizeof("Product id not in the order or out of stock\n"));
        return;
    }

    c.products[i].qty = qty;
    write(nsd, "Update successful\n", sizeof("Update successful\n"));
    cartRecordLock(fd_cart, lock_cart, offset, 2);
    write(fd_cart, &c, sizeof(struct cart));
    unlock(fd_cart, lock_cart);
}

void addProductToCart(int fd, int fd_cart, int fd_customers, int nsd)
{
    int cust_id = -1;
    read(nsd, &cust_id, sizeof(int));
    int offset = getOffset(cust_id, fd_customers);

    write(nsd, &offset, sizeof(int));

    if (offset == -1)
    {
        return;
    }

    struct flock lock_cart;

    int i = -1;
    cartRecordLock(fd_cart, lock_cart, offset, 1);
    struct cart c;
    read(fd_cart, &c, sizeof(struct cart));

    struct flock lock_prod;
    inventoryReadLock(fd, lock_prod);

    struct product p;
    read(nsd, &p, sizeof(struct product));

    int prod_id = p.id;
    int qty = p.qty;

    struct product p1;
    int found = 0;
    while (read(fd, &p1, sizeof(struct product)))
    {
        if (p1.id == p.id)
        {
            if (p1.qty >= p.qty)
            {
                found = 1;
                break;
            }
        }
    }
    unlock(fd_cart, lock_cart);
    unlock(fd, lock_prod);

    if (!found)
    {
        write(nsd, "Product id invalid or out of stock\n", sizeof("Product id invalid or out of stock\n"));
        return;
    }

    int flg = 0;

    int flg1 = 0;
    for (int i = 0; i < PROD_LIMIT; i++)
    {
        if (c.products[i].id == p.id)
        {
            flg1 = 1;
            break;
        }
    }

    if (flg1)
    {
        write(nsd, "Product already exists in cart\n", sizeof("Product already exists in cart\n"));
        return;
    }

    for (int i = 0; i < PROD_LIMIT; i++)
    {
        if (c.products[i].id == -1 || c.products[i].qty <= 0)
        {
            flg = 1;
            c.products[i].id = p.id;
            c.products[i].qty = p.qty;
            strcpy(c.products[i].name, p1.name);
            c.products[i].price = p1.price;
            break;
        }
    }

    if (!flg)
    {
        write(nsd, "Cart limit reached\n", sizeof("Cart limit reached\n"));
        return;
    }

    write(nsd, "Item added to cart\n", sizeof("Item added to cart\n"));

    cartRecordLock(fd_cart, lock_cart, offset, 2);
    write(fd_cart, &c, sizeof(struct cart));
    unlock(fd_cart, lock_cart);
}

void generateAdminLog(int fd_admin, int fd)
{
    struct flock lock;
    inventoryReadLock(fd, lock);
    write(fd_admin, "Current Inventory:\n", strlen("Current Inventory:\n"));
    write(fd_admin, "ProductID\tProductName\tQuantity\tPrice\n", strlen("ProductID\tProductName\tQuantity\tPrice\n"));

    lseek(fd, 0, SEEK_SET);
    struct product p;
    while (read(fd, &p, sizeof(struct product)))
    {
        if (p.id != -1)
        {
            char temp[100];
            sprintf(temp, "%d\t%s\t%d\t%d\n", p.id, p.name, p.qty, p.price);
            write(fd_admin, temp, strlen(temp));
        }
    }
    unlock(fd, lock);
}

void payment(int fd_inventory, int fd_cart, int fd_customers, int nsd)
{
    int cust_id;
    read(nsd, &cust_id, sizeof(int));

    int offset = getOffset(cust_id, fd_customers);

    write(nsd, &offset, sizeof(int));
    if (offset == -1)
    {
        return;
    }

    struct flock lock_cart;
    cartRecordLock(fd_cart, lock_cart, offset, 1);

    struct cart c;
    read(fd_cart, &c, sizeof(struct cart));
    unlock(fd_cart, lock_cart);
    write(nsd, &c, sizeof(struct cart));

    int total = 0;

    for (int i = 0; i < PROD_LIMIT; i++)
    {

        if (c.products[i].id != -1)
        {
            write(nsd, &c.products[i].qty, sizeof(int));

            struct flock lock_prod;
            inventoryReadLock(fd_inventory, lock_prod);
            lseek(fd_inventory, 0, SEEK_SET);

            struct product p;
            int flg = 0;
            while (read(fd_inventory, &p, sizeof(struct product)))
            {

                if (p.id == c.products[i].id && p.qty > 0)
                {
                    int min;
                    if (c.products[i].qty >= p.qty)
                    {
                        min = p.qty;
                    }
                    else
                    {
                        min = c.products[i].qty;
                    }

                    flg = 1;
                    write(nsd, &min, sizeof(int));
                    write(nsd, &p.price, sizeof(int));
                    break;
                }
            }

            unlock(fd_inventory, lock_prod);

            if (!flg)
            {
                // product got deleted midway
                int val = 0;
                write(nsd, &val, sizeof(int));
                write(nsd, &val, sizeof(int));
            }
        }
    }

    char ch;
    read(nsd, &ch, sizeof(char));

    for (int i = 0; i < PROD_LIMIT; i++)
    {

        struct flock lock_prod;
        inventoryReadLock(fd_inventory, lock_prod);
        lseek(fd_inventory, 0, SEEK_SET);

        struct product p;
        while (read(fd_inventory, &p, sizeof(struct product)))
        {

            if (p.id == c.products[i].id)
            {
                int min;
                if (c.products[i].qty >= p.qty)
                {
                    min = p.qty;
                }
                else
                {
                    min = c.products[i].qty;
                }
                unlock(fd_inventory, lock_prod);
                inventoryWriteLock(fd_inventory, lock_prod);
                p.qty = p.qty - min;

                write(fd_inventory, &p, sizeof(struct product));
                unlock(fd_inventory, lock_prod);
            }
        }

        unlock(fd_inventory, lock_prod);
    }

    cartRecordLock(fd_cart, lock_cart, offset, 2);

    for (int i = 0; i < PROD_LIMIT; i++)
    {
        c.products[i].id = -1;
        strcpy(c.products[i].name, "");
        c.products[i].price = -1;
        c.products[i].qty = -1;
    }

    write(fd_cart, &c, sizeof(struct cart));
    write(nsd, &ch, sizeof(char));
    unlock(fd_cart, lock_cart);

    read(nsd, &total, sizeof(int));
    read(nsd, &c, sizeof(struct cart));

    int fd_rec = open("receipt.txt", O_CREAT | O_RDWR, 0666);
    write(fd_rec, "ProductID\tProductName\tQuantity\tPrice\n", strlen("ProductID\tProductName\tQuantity\tPrice\n"));
    char temp[100];
    for (int i = 0; i < PROD_LIMIT; i++)
    {
        if (c.products[i].id != -1)
        {
            sprintf(temp, "%d\t%s\t%d\t%d\n", c.products[i].id, c.products[i].name, c.products[i].qty, c.products[i].price);
            write(fd_rec, temp, strlen(temp));
        }
    }
    sprintf(temp, "Total - %d\n", total);
    write(fd_rec, temp, strlen(temp));
    close(fd_rec);
}

int main()
{
    printf("Server starting...\n");

    int fd_customers = open("customers.txt", O_RDWR | O_CREAT, 0666);
    int fd_admin = open("adminLog.txt", O_RDWR | O_CREAT, 0666);
    int fd_inventory = open("inventory.txt", O_RDWR | O_CREAT, 0666);
    int fd_cart = open("carts.txt", O_RDWR | O_CREAT, 0666);

    int sd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in serv, cli;

    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = INADDR_ANY;
    serv.sin_port = htons(6767);

    int opt = 1;
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        perror("Error: ");
        return -1;
    }

    if (bind(sd, (struct sockaddr *)&serv, sizeof(serv)) == -1)
    {
        perror("Error: ");
        return -1;
    }

    if (listen(sd, 5) == -1)
    {
        perror("Error: ");
        return -1;
    }

    lseek(fd_admin, 0, SEEK_END);

    int size = sizeof(cli);
    printf("Server up and running.\n");

    while (1)
    {
        int nsd = accept(sd, (struct sockaddr *)&cli, &size);
        if (nsd == -1)
        {
            return -1;
        }

        if (!fork())
        {
            printf("Connection with client successful\n");
            close(sd);

            int user;
            read(nsd, &user, sizeof(int));
            char ch;

            if (user == 1)
            {
                while (1)
                {
                    read(nsd, &ch, sizeof(char));

                    lseek(fd_inventory, 0, SEEK_SET);
                    lseek(fd_cart, 0, SEEK_SET);
                    lseek(fd_customers, 0, SEEK_SET);

                    if (ch == 'a')
                    {
                        registerCustomer(fd_cart, fd_customers, nsd);
                    }
                    else if (ch == 'b')
                    {
                        listInventory(fd_inventory, nsd);
                    }

                    else if (ch == 'c')
                    {
                        viewCart(fd_cart, nsd, fd_customers);
                    }

                    else if (ch == 'd')
                    {
                        addProductToCart(fd_inventory, fd_cart, fd_customers, nsd);
                    }

                    else if (ch == 'e')
                    {
                        editCartItem(fd_inventory, fd_cart, fd_customers, nsd);
                    }

                    else if (ch == 'f')
                    {
                        payment(fd_inventory, fd_cart, fd_customers, nsd);
                    }

                    else if (ch == 'g')
                    {
                        close(nsd);
                        break;
                    }
                }
                printf("Connection terminated\n");
            }
            else if (user == 2)
            {
                while (1)
                {
                    read(nsd, &ch, sizeof(ch));

                    lseek(fd_cart, 0, SEEK_SET);
                    lseek(fd_customers, 0, SEEK_SET);
                    lseek(fd_inventory, 0, SEEK_SET);

                    if (ch == 'a')
                    {
                        addProducts(fd_inventory, nsd, fd_admin);
                    }
                    else if (ch == 'b')
                    {
                        int id;
                        read(nsd, &id, sizeof(int));
                        deleteProduct(fd_inventory, nsd, id, fd_admin);
                    }
                    else if (ch == 'c')
                    {
                        updateProduct(fd_inventory, nsd, 1, fd_admin);
                    }

                    else if (ch == 'd')
                    {
                        updateProduct(fd_inventory, nsd, 2, fd_admin);
                    }

                    else if (ch == 'e')
                    {
                        listInventory(fd_inventory, nsd);
                    }

                    else if (ch == 'f')
                    {
                        close(nsd);
                        generateAdminLog(fd_admin, fd_inventory);
                        break;
                    }
                    else
                    {
                        continue;
                    }
                }
            }
            printf("Connection terminated\n");
        }
        else
        {
            close(nsd);
        }
    }
}