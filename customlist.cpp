#include "customlist.hpp"
#include "cpu.hpp"

#include <iostream>

customList::customList():
    head(nullptr)
  , tail(nullptr)
{

}

customList::Item *customList::operator[](const int index)
{
    //если элементов нет в списке
    if (head == nullptr)
    {
        return nullptr;
    }
    Item* element = head;
    for (int i = 0; i < index; i++)
    {
        element = element->next;
        if (!element) return nullptr; //если индекс был указан
        //больше размера списка
    }
    return element;
}

customList::~customList()
{
    while(head) //пока по адресу начала списка что-то есть
    {
        tail = head->next;
        delete head;
        head = tail;
    }
}

void customList::addToList(CPU &adCPU)
{
    Item *temp = new Item; //выделение памяти под новый элемент списка
    temp->next = nullptr; //изначально по следующему адресу null_ptr
    temp->cpu = adCPU; //записываем значение в структуру

    if (head != nullptr) //если список не пустой
    {
        temp->prev = tail; //указываем адрес на предыдущий элемент в соотв. поле
        tail->next = temp; //указываем адрес следующего за хвостом элемента
        tail = temp; //меняем адрес хвоста
    }
    else //если список пустой
    {
        temp->prev = nullptr; //предыдущий элемент указывает в null_ptr
        head = temp;
        tail = temp; //голова=хвост=тот элемент, что сейчас добавили
    }
}

void customList::deleteFromList(int i)
{
    Item* cur = this->operator[](i);
    if (cur==nullptr) return;
    Item* r;
    if ((r = cur->prev) != nullptr)
    {
        r->next = cur->next;
    }
    else
    {
        head = cur->next;
    }
    if ((r = cur->next) != nullptr)
    {
        r->prev = cur->prev;
    }
    else
    {
        tail = cur->prev;
    }
    cur->~Item();
}
