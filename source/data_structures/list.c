
#include "list.h"

void List_Init(List *list, void (*destroy)(void *data))
{
    ZeroMemory(list, sizeof(List));
    list->destroy = destroy;
}

void List_Clear(List *list)
{
    while (list->length > 0)
    {
        // NOLINTNEXTLINE
        List_RemoveElement(list, list->tail);
    }
}

int List_Insert(List *list, const void *data)
{
    if (!List_InsertNext(list, list->tail, data))
    {
        return 0;
    }

    return 1;
}

int List_InsertAt(List *list, const void *data, int position)
{
    ListElmt *referenceElement;
    if (!List_GetElementAtPosition(list, &referenceElement, position))
    {
        return FALSE;
    }

    if (!List_InsertNext(list, referenceElement, data))
    {
        return FALSE;
    }

    return TRUE;
}

int List_InsertNext(List *list, ListElmt *element, const void *data)
{
    ListElmt *newElement;

    if (element == NULL && list->length != 0)
    {
        return FALSE;
    }

    if ((newElement = malloc(sizeof(ListElmt))) == NULL)
    {
        return FALSE;
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
    return TRUE;
}

int List_InsertPrevious(List *list, ListElmt *element, const void *data)
{
    ListElmt *newElement;

    if (element == NULL && list->length != 0)
    {
        return FALSE;
    }

    if ((newElement = malloc(sizeof(ListElmt))) == NULL)
    {
        return FALSE;
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

    return TRUE;
}

int List_RemovePosition(List *list, int position)
{
    ListElmt *referenceElement;
    if (!List_GetElementAtPosition(list, &referenceElement, position))
    {
        return FALSE;
    }

    if (!List_RemoveElement(list, referenceElement))
    {
        return FALSE;
    }

    return TRUE;
}

/// @brief Removes the element from the list.
/// Does *not* check if the element is in the list.
/// Do not pass elements that you are not sure are in the list or not.
/// @param list The list to remove the element from.
/// @param element The element to remove from the list.
/// @return Returns 0 if the list is empty or the element is NULL, returns 1 on success.
int List_RemoveElement(List *list, ListElmt *element)
{
    if (element == NULL || list->length == 0)
    {
        return FALSE;
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

    return TRUE;
}

/// @brief Attempts to remove an element from the passed list based on data pointer comparison.
/// @param list The list to remove the element from.
/// @param data The data pointer to compare against the elements in the list.
/// @return Returns 0 if unable to remove the element. Otherwise returns 1.
int List_RemoveElementWithMatchingData(List *list, void *data)
{
    ListElmt *referenceElement = NULL;
    if (!List_GetElementWithMatchingData(list, &referenceElement, data))
    {
        return 0;
    }

    return List_RemoveElement(list, referenceElement);
}

/// @brief Finds the first occurence of the data pointer in the list.
/// Do not use this if you are cycling through a list, manually assign ListElmt through a while loop.
/// @param list The list to search through
/// @param data The data pointer to search for
/// @return -1 if data not found, otherwise returns element position holding the data.
int List_GetDataPosition(List *list, void *data)
{
    if (list->length < 1)
    {
        return -1;
    }

    unsigned int i;
    ListElmt *referenceElement = list->head;
    for (i = 0; i < list->length; i++)
    {
        if (data == referenceElement->data)
        {
            return (int)i;
        }
    }

    return -1;
}

short List_GetDataAtPosition(List *list, void **data, unsigned int position)
{
    if (position > list->length)
    {
        return FALSE;
    }

    ListElmt *referenceElement = list->head;
    unsigned int i;
    for (i = 0; i < position; i++)
    {
        referenceElement = referenceElement->next;
    }

    *data = referenceElement->data;

    return TRUE;
}

/// @brief Finds an element in the list
/// @param list The list to search through
/// @param element The element to search for
/// @return -1 if element not found, otherwise returns position of element
/// Do not use this if you are cycling through a list, manually assign ListElmt through a while loop.
int List_GetElementPosition(List *list, ListElmt *element)
{
    if (list->length < 1)
    {
        return -1;
    }

    unsigned int i;
    ListElmt *referenceElement = list->head;
    for (i = 0; i < list->length; i++)
    {
        if (element == referenceElement)
        {
            return (int)i;
        }
    }

    return -1;
}

short List_GetElementAtPosition(List *list, ListElmt **element, unsigned int position)
{
    if (position > list->length)
    {
        return FALSE;
    }

    ListElmt *referenceElement = list->head;
    unsigned int i;
    for (i = 0; i < position; i++)
    {
        referenceElement = referenceElement->next;
    }

    *element = referenceElement;

    return TRUE;
}

/// @brief Cycles through the elements in the list and compares pointer value of data argument to the element's data variable.
/// @param list The list to cycle through.
/// @param element Reference to an element pointer that will be assigned to if matching data found.
/// @param data The data we look for in the list.
/// @return Returns 0 if not found, 1 if found data.
short List_GetElementWithMatchingData(List *list, ListElmt **element, void *data)
{

    ListElmt *referenceElement = list->head;
    while (referenceElement != NULL && data != referenceElement->data)
    {
        referenceElement = referenceElement->next;
    }

    if (referenceElement == NULL)
    {
        return FALSE;
    }

    *element = referenceElement;

    return TRUE;
}

void List_GetAsArray(List *list, void **returnedArray)
{
    void **array = malloc(list->length * sizeof(void *));
    unsigned int counter = 0;
    ListElmt *referenceElement = list->head;
    while (referenceElement != NULL)
    {
        array[counter] = referenceElement->data;
        referenceElement = referenceElement->next;
        counter++;
    }

    (*returnedArray) = array;
}

void List_FreeOnRemove(void *data)
{
    free(data);
}

void List_CloseHandleOnRemove(void *data)
{
    CloseHandle(data);
}
