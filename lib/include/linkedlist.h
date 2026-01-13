#ifndef linkedlist_header_h
#define linkedlist_header_h

#include <datanode.h>
#include <memory>
#include <utility>

namespace SimpleSql {

    template<typename T>
    class linkedlist {
    private:
        std::unique_ptr<SimpleSql::datanode<T>> m_first = nullptr;

    public:
        SimpleSql::datanode<T>* first() const { return m_first.get(); }
        void push_front(T data) {
            auto new_datanode = std::make_unique<SimpleSql::datanode<T>>(data);
            if (m_first) {
                m_first->m_previous = new_datanode.get();
                new_datanode->m_next = std::move(m_first);
            }
            m_first = std::move(new_datanode);
        }

    };
}

#endif