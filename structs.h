#include <stdio.h>
#define PROD_LIMIT 20

struct product
{
    int id;
    char name[50];
    int qty;
    int price;
};
struct cart
{
    int customer_id;
    struct product products[PROD_LIMIT];
};
struct cart_index
{
    int customer_id;
    int offset;
};