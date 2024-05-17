
#include "../headers/list.h"

void list_init(List *list, void (*destroy)(void *data))
{
    list->length = 0;
    list->destroy = destroy;
    list->head = NULL;
    list->tail = NULL;
}

void list_clear(List *list)
{
    while (list->length > 0)
    {
        // NOLINTNEXTLINE
        list_remove_element(list, list->tail);
    }

    list_init(list, NULL);
}

int list_insert(List *list, const void *data)
{
    if (!list_ins_next(list, list->tail, data))
    {
        return 0;
    }

    return 1;
}

int list_insert_at(List *list, const void *data, int position)
{
    ListElmt *referenceElement;
    if (!list_get_element_at_position(list, &referenceElement, position))
    {
        return 0;
    }

    if (!list_ins_next(list, referenceElement, data))
    {
        return 0;
    }

    return 1;
}

int list_ins_next(List *list, ListElmt *element, const void *data)
{
    ListElmt *newElement;

    if (element == NULL && list->length != 0)
    {
        return 0;
    }

    if ((newElement = malloc(sizeof(ListElmt))) == NULL)
    {
        return 0;
    }

    newElement->data = (void *)data;

    if (list->length == 0)
    {
        list->head = newElement;
        list->head->prev = NULL;
        list->head->next = NULL;
        list->tail = newElement;
    }
    else
    {
        newElement->next = element->next;
        newElement->prev = element;

        if (element->next == NULL)
        {
            list->tail = newElement;
        }
        else
        {
            element->next->prev = newElement;
        }

        element->next = newElement;
    }

    list->length++;
    return 1;
}

int list_insert_prev(List *list, ListElmt *element, const void *data)
{
    ListElmt *newElement;

    if (element == NULL && list->length != 0)
    {
        return 0;
    }

    if ((newElement = malloc(sizeof(ListElmt))) == NULL)
    {
        return 0;
    }

    newElement->data = (void *)data;

    if (list->length == 0)
    {
        list->head = newElement;
        list->head->prev = NULL;
        list->head->next = NULL;
        list->tail = newElement;
    }
    else
    {
        newElement->next = element;
        newElement->prev = element->prev;

        if (element->prev == NULL)
        {
            list->head = newElement;
        }
        else
        {
            element->prev->next = newElement;
        }

        element->prev = newElement;
    }

    list->length++;

    return 1;
}

int list_remove_position(List *list, int position)
{
    ListElmt *referenceElement;
    if (!list_get_element_at_position(list, &referenceElement, position))
    {
        return 0;
    }

    if (!list_remove_element(list, referenceElement))
    {
        return 0;
    }

    return 1;
}

/// @brief Removes the element from the list.
/// Does *not* check if the element is in the list.
/// Do not pass elements that you are not sure are in the list or not.
/// @param list The list to remove the element from.
/// @param element The element to remove from the list.
/// @return Returns 0 if the list is empty or the element is NULL, returns 1 on success.
int list_remove_element(List *list, ListElmt *element)
{
    if (element == NULL || list->length == 0)
    {
        return 0;
    }

    if (element == list->head)
    {
        list->head = element->next;

        if (list->head == NULL)
        {
            list->tail = NULL;
        }
        else
        {
            element->next->prev = NULL;
        }
    }
    else
    {
        element->prev->next = element->next;

        if (element->next == NULL)
        {
            list->tail = element->prev;
        }
        else
        {
            element->next->prev = element->prev;
        }
    }

    if (list->destroy != NULL)
    {
        list->destroy(element->data);
    }

    free(element);
    list->length--;

    return 1;
}

/// @brief Attempts to remove an element from the passed list based on data pointer comparison.
/// @param list The list to remove the element from.
/// @param data The data pointer to compare against the elements in the list.
/// @return Returns 0 if unable to remove the element. Otherwise returns 1.
int list_remove_element_with_matching_data(List *list, void *data)
{
    ListElmt *referenceElement = NULL;
    if (!list_get_element_with_matching_data(list, &referenceElement, data))
    {
        return 0;
    }

    return list_remove_element(list, referenceElement);
}

/// @brief Finds an element in the list
/// @param list The list to search through
/// @param element The element to search for
/// @return -1 if element not found, otherwise returns position of element
/// Do not use this if you are cycling through a list, manually assign ListElmt through a while loop.
int list_get_element_position(List *list, ListElmt *element)
{
    if (list->length < 1)
    {
        return -1;
    }

    int i;
    ListElmt *referenceElement = list->head;
    for (i = 1; i < list->length; i++)
    {
        if (element == referenceElement)
        {
            return i - 1;
        }
    }

    return -1;
}

/// @brief Finds the first occurence of the data pointer in the list.
/// Do not use this if you are cycling through a list, manually assign ListElmt through a while loop.
/// @param list The list to search through
/// @param data The data pointer to search for
/// @return -1 if data not found, otherwise returns element position holding the data.
int list_get_data_position(List *list, void *data)
{
    if (list->length < 1)
    {
        return -1;
    }

    int i;
    ListElmt *referenceElement = list->head;
    for (i = 1; i < list->length; i++)
    {
        if (data == referenceElement->data)
        {
            return i - 1;
        }
    }

    return -1;
}

int list_get_element_at_position(List *list, ListElmt **element, int position)
{
    if (position > list->length)
    {
        return 0;
    }

    ListElmt *referenceElement = list->head;
    int i;
    for (i = 1; i < position; i++)
    {
        referenceElement = referenceElement->next;
    }

    *element = referenceElement;

    return 1;
}

/// @brief Cycles through the elements in the list and compares pointer value of data argument to the element's data variable.
/// @param list The list to cycle through.
/// @param element Reference to an element pointer that will be assigned to if matching data found.
/// @param data The data we look for in the list.
/// @return Returns 0 if not found, 1 if found data.
int list_get_element_with_matching_data(List *list, ListElmt **element, void *data)
{

    ListElmt *referenceElement = list->head;
    while (referenceElement != NULL && data != referenceElement->data)
    {
        referenceElement = referenceElement->next;
    }

    if (referenceElement == NULL)
    {
        return 0;
    }

    *element = referenceElement;

    return 1;
}

void list_free_on_remove(void *data)
{
    free(data);
}
