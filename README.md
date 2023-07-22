# OnlineStore
An online store implemented using system calls and Linux programming in C as a part of the Operating Systems course.

# Client Menu :
You can login as one of the following:
1. Admin (has permissions to update the products and add products to the inventory, and also to delete products)
2. User (has permissions only limited to the items in his cart, to add items, remove items and to pay for his cart)

# Admin Functionalities:
1. Add/Del a product
2. Update the quantity of a new product, or update the price

# User Functionalities:
1. See the list of products the server has.
2. See his own cart.
3. Add/delete items from his cart
4. Update quantities in his cart
5. Pay for the items in his cart, and generate a receipt for the same.

# Program Architecture
1. There are 2 programs - Server.c and Client.c. Server.c is the Server code, while Client.c is the Client code, which allows you to login as a user or as the admin.
2. The program uses sockets to communicate between the server and client, and file locking while accessing the files which contain data.
3. customers.txt contains the list of customers registered with the program, orders.txt contains the cart for each customer, records.txt contains the products in the inventory, and receipt.txt contains the recept generated once payment for a customer is completed.

# Instructions to run the program
Open a terminal, and run the following commands 

```
gcc -o server Server.c
./server
```

In a separate terminal, run the following commands
```
gcc -o client Client.c
./client
```

Now you can use the user menu or admin menu as directed by the program to perform operations on the products or customers.

Kindly refer to the project report for details on implementation.
