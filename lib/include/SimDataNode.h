#ifndef SimDataNode_header_h
#define SimDataNode_header_h

// STL stuff
#include <memory>
#include <utility>

class linkedlist;

namespace SimpleSql {

    template<typename T>
    class datanode {
    private:
        T m_data = nullptr;
        std::unique_ptr<datanode> m_next = nullptr;
        datanode* m_previous = nullptr;

        friend class linkedlist;
    public:
        datanode(T d = nullptr) : m_data(d) {}
        ~datanode() {}

        // getters
        datanode* next() const { return m_next.get(); }
        datanode* prev() const { return m_previous; }
        T data() const { return m_data; }

        // linker
        void insert_after(std::unique_ptr<datanode> new_node) {
            if (m_next) {
                m_next->m_previous = new_node.get();
                new_node->m_next = std::move(m_next);
            }

            new_node->m_previous = this;
            m_next = std::move(new_node);
        }
    };
}

#endif