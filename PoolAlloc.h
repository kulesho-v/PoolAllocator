#pragma once
#include <mutex>

template<int PoolSize, int ChunkSize,
    class Enable = typename std::enable_if<PoolSize%ChunkSize == 0 && ChunkSize >= sizeof(void*), bool>::type>
class PoolAlloc
{
    struct Node
    {
        Node* Next = nullptr;
    };

    class LinkedList
    {
    public:
        LinkedList() = default;
        ~LinkedList() = default;

        LinkedList(const LinkedList& OtherLL) = delete;
        LinkedList& operator=(const LinkedList& OtherLL) = delete;

        void Push(Node* NewNode)
        {
            std::lock_guard<std::mutex> Lock(HeadMutex);

            NewNode->Next = Head;
            Head = NewNode;
        }

        Node* Pop()
        {
            std::lock_guard<std::mutex> Lock(HeadMutex);

            Node* TopElement = Head;
            if (Head)
            {
                Head = Head->Next;
            }

            return TopElement;
        }

    private:
        Node* Head = nullptr;
        std::mutex HeadMutex;
    };

public:
    PoolAlloc();
    virtual ~PoolAlloc();

    void* Allocate();
    void Free(void* MemToFree);

protected:
    virtual void Init();

private:
    std::once_flag InitFlag;

    char* PoolMemory = nullptr;
    LinkedList LList;
};

template<int PoolSize, int ChunkSize, class Enable>
inline PoolAlloc<PoolSize, ChunkSize, Enable>::PoolAlloc()
    : PoolMemory(new char[PoolSize])
{
    std::call_once(InitFlag, &PoolAlloc<PoolSize, ChunkSize, Enable>::Init, this);
}

template<int PoolSize, int ChunkSize, class Enable>
inline PoolAlloc<PoolSize, ChunkSize, Enable>::~PoolAlloc()
{
    delete[] PoolMemory;
}

template<int PoolSize, int ChunkSize, class Enable>
inline void* PoolAlloc<PoolSize, ChunkSize, Enable>::Allocate()
{
    return LList.Pop();
}

template<int PoolSize, int ChunkSize, class Enable>
inline void PoolAlloc<PoolSize, ChunkSize, Enable>::Free(void* MemToFree)
{
    LList.Push((Node*)MemToFree);
}

template<int PoolSize, int ChunkSize, class Enable>
inline void PoolAlloc<PoolSize, ChunkSize, Enable>::Init()
{
    char* Start = nullptr;
    char* End = nullptr;

    for (Start = PoolMemory, End = PoolMemory + (PoolSize - ChunkSize); Start < End; Start += ChunkSize)
    {
        LList.Push((Node*)Start);
    }
}
