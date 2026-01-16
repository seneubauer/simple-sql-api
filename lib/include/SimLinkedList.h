#ifndef SimLinkedList_header_h
#define SimLinkedList_header_h

// SimQL stuff
#include <SimDataNode.h>

// STL stuff
#include <memory>
#include <utility>

namespace SimpleSql {

    template<typename T>
    class linkedlist {
    private:
        std::unique_ptr<SimpleSql::SimDataNode<T>> m_first = nullptr;

    public:
        SimpleSql::SimDataNode<T>* first() const { return m_first.get(); }
        void push_front(T data) {
            auto new_datanode = std::make_unique<SimpleSql::SimDataNode<T>>(data);
            if (m_first) {
                m_first->m_previous = new_datanode.get();
                new_datanode->m_next = std::move(m_first);
            }
            m_first = std::move(new_datanode);
        }

    };
}

#endif