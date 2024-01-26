#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

class allocator {
private:
    struct Node {
        int Tid;
        int size;
        int index;
        Node* next;

        Node(int id, int s, int idx) : Tid(id), size(s), index(idx), next(nullptr) {}
    };

    Node* head;
    std::mutex mtx;

public:

    allocator() : head(nullptr) {}

    int initHeap(int s);

    int myMalloc(int id, int s);
    
    int myFree(int id, int idx);

    void print();

    void executionDone();

    ~allocator();

};

int allocator::initHeap(int s) { // create a free node in list with size s

        head = new Node(-1, s, 0);
        
        print();

        return 1;
}

int allocator::myMalloc(int id, int s) {  //traverse each node in list
				//if free node and has enough size split from beginning of the free space
        std::lock_guard<std::mutex> lock(mtx);

        Node* curr = head;
        Node* prev = nullptr;

        while (curr != nullptr) {
            if (curr->Tid == -1 && curr->size >= s) {
                
                int remainingSize = curr->size - s;

                if (remainingSize > 0) {
                    
                    Node* allocatedNode = new Node(id, s, curr->index);
                    Node* newFreeNode = new Node(-1, remainingSize, curr->index + s);

                    allocatedNode->next = newFreeNode;
                    newFreeNode->next = curr->next;

                    if (prev == nullptr) {
                        head = allocatedNode;
                    } else {
                        prev->next = allocatedNode;
                    }
                } else {
                    
                    curr->Tid = id;
                }

                
                std::cout << "Allocated for thread " << id << std::endl;
                print();

                return curr->index;
            }

            prev = curr;
            curr = curr->next;
        }

        
        std::cout << "Can not allocate, requested size " << s 
            << " for thread " << id <<  " is bigger than remaining size" << std::endl;
        print();

        return -1;
}

int allocator::myFree(int id, int idx) { // free the node
				// if prev node is free coalesce
				// if next node is free coalesce
        std::lock_guard<std::mutex> lock(mtx);

        Node* curr = head;
        Node* prev = nullptr;

        while (curr != nullptr) {
            if (curr->Tid == id && curr->index == idx) {
                
                curr->Tid = -1;  

                if (prev != nullptr && prev->Tid == -1) {
                    prev->size += curr->size;
                    prev->next = curr->next;
                    curr->next == nullptr;
                    delete curr;
                    curr = prev;
                }

                Node* nextNode = curr->next;
                if (nextNode != nullptr && nextNode->Tid == -1) {
                    curr->size += nextNode->size;
                    curr->next = nextNode->next;
                    delete nextNode;
                }

                
                std::cout << "Freed for thread " << id << std::endl;
                print();

                return 1;
            }

            prev = curr;
            curr = curr->next;
        }

        
        std::cout << "Freeing failed for thread " << id << std::endl;
        print();

        return -1;
}

void allocator::print() {
        Node* temp = head;

        while (temp != nullptr) {
        
            std::cout << "[" << temp->Tid << "][" << temp->size << "][" << temp->index << "]";

            if (temp->next != nullptr) {
                std::cout << "---";
            }
            
            temp = temp->next;
        }

        std::cout << std::endl;
}

void allocator::executionDone(){

    std::cout << "Execution is done\n";
    this->print();
}

allocator::~allocator() {
    Node* current = head;
    while (current != nullptr) {
        Node* next = current->next;
        delete current;
        current = next;
    }
    head = nullptr;
}


/*
void Malloc(allocator& a, int threadID, int size) {
    a.myMalloc(threadID, size);
    
}

void Free(allocator& a, int threadID, int index) {
    a.myFree(threadID, index);
}


int main() {

    allocator a;
    a.initHeap(100);

    a.myMalloc(0, 10);
    a.myMalloc(1, 15);
    a.myMalloc(2, 20);
    a.myMalloc(3, 25);
    a.myMalloc(4, 30);


    a.myFree(0, 0);
    a.myFree(2, 25);
    a.myFree(1,10);
    a.myMalloc(0,1);
    a.executionDone();

    
    return 0;
}
*/



