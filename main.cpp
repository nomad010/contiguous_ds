#include <algorithm>
#include <vector>

enum ContainerState
{
    Ready = 0,
    Modified,
};

template <typename T, int BufferSize = 64>
class contiguous_set
{
protected:
    enum BufferOperation
    {
        InsertItem,
        DeleteItem
    };
    
    class BufferItem
    {
    public:
        BufferOperation operation;
        T value;
        
        BufferItem()
        {
        }
        
        BufferItem(BufferOperation operation, T value) : operation(operation), value(value)
        {
        }
        
        bool operator<(const BufferItem& item) const
        {
            if(operation == item.operation)
                return value < item.value;
            else
                return operation < item.operation;
        }
    };
    
    BufferItem op_buffer[BufferSize];
    int buffer_ops;
    
    ContainerState state;
    std::vector<T> items;
    
public:
    contiguous_set() : buffer_ops(0), state(Ready)
    {
    }
    
    contiguous_set(contiguous_set<T>& other)
    {
        other.make_ready();
        items.resize(items.size());
        std::copy(other.items.begin(), other.items.end(), items.begin());
    }
    
    typename std::vector<T>::iterator begin() noexcept
    {
        make_ready();
        return items.begin();
    }
    
    typename std::vector<T>::iterator end() noexcept
    {
        make_ready();
        return items.end();
    }
    
    void make_ready()
    {
        if(buffer_ops == 0)
        {
            state = Ready;
            return;
        }
        
        /// Want to reorder the ops so that the effect is the same, but more efficient.
        /// InsertItem/DeleteItem pairs of the same value are nuked.
        
        BufferItem optimised_op_buffer[BufferSize];
        int write_command_pos = 0;
        
        bool handled[BufferSize];
        std::fill(handled, handled + BufferSize, false);
        
        for(int i = 0; i < buffer_ops; ++i)
        {
            if(handled[i])
                continue;
            handled[i] = true;
            
            if(op_buffer[i].operation == InsertItem)
            {
                bool eventual_add = true;
                                                          
                for(int j = i + 1; j < buffer_ops; ++j)
                {
                    if(op_buffer[i].value == op_buffer[j].value)
                    {
                        if(op_buffer[j].operation == InsertItem)
                        {
                            eventual_add = true;
                            handled[j] = true;
                        }
                        else if(op_buffer[j].operation == DeleteItem)
                        {
                            eventual_add = false;
                            handled[j] = true;
                        }
                    }
                }
                
                if(eventual_add && !std::binary_search(items.begin(), items.end(), op_buffer[i].value))
                    optimised_op_buffer[write_command_pos++] = op_buffer[i];
            }
            else if(op_buffer[i].operation == DeleteItem)
            {
                bool eventual_delete = true;
                                                          
                for(int j = i + 1; j < buffer_ops; ++j)
                {
                    if(op_buffer[i].value == op_buffer[j].value)
                    {
                        if(op_buffer[j].operation == InsertItem)
                        {
                            eventual_delete = false;
                            handled[j] = true;
                        }
                        else if(op_buffer[j].operation == DeleteItem)
                        {
                            eventual_delete = true;
                            handled[j] = true;
                        }
                    }
                }
                
                if(eventual_delete && std::binary_search(items.begin(), items.end(), op_buffer[i].value))
                    optimised_op_buffer[write_command_pos++] = op_buffer[i];
            }
        }
        std::sort(optimised_op_buffer, optimised_op_buffer + write_command_pos);
        int inserts = 0;
        for(; inserts < write_command_pos; ++inserts)
            if(optimised_op_buffer[inserts].operation == DeleteItem)
                break;
        int old_size = items.size();
        items.resize(old_size + inserts);
        
        for(int i = 0; i < inserts; ++i)
            items[old_size + i] = optimised_op_buffer[i].value;
        std::inplace_merge(items.begin(), items.end() - inserts, items.end());
        
        /// Now delete
        int write_item_pos = 0;
        int delete_pos = inserts;
        
        for(int i = 0; i < int(items.size()); ++i)
        {

            if(delete_pos < write_command_pos && items[i] == optimised_op_buffer[delete_pos].value)
                ++delete_pos;
            else
                items[write_item_pos++] = items[i];
        }
        items.resize(write_item_pos);
        buffer_ops = 0;
        state = Ready;
    }
    
    bool empty() noexcept
    {
        make_ready();
        return items.empty();
    }
    
    typename std::vector<T>::size_type size() noexcept
    {
        make_ready();
        return items.size();
    }
    
    typename std::vector<T>::size_type max_size() const noexcept
    {
        return items.max_size();
    }
    
    void insert(const T& val)
    {
        if(buffer_ops == BufferSize)
            make_ready();
        op_buffer[buffer_ops++] = BufferItem(InsertItem, val);
    }

    template <class InputIterator>
    void insert(InputIterator first, InputIterator last)
    {
        while(first != last)
        {
            insert(*first);
            ++first;
        }
    }
    
    void erase(const T& val)
    {
        if(buffer_ops == BufferSize)
            make_ready();
        op_buffer[buffer_ops++] = BufferItem(DeleteItem, val);
    }
    
    void swap(contiguous_set<T>& x)
    {
        for(int i = 0; i < BufferSize; ++i)
            swap(op_buffer[i], x.op_buffer[i]);
        swap(buffer_ops, x.buffer_ops);
        swap(state, x.state);
        items.swap(x.items);
    }
    
    void clear() noexcept
    {
        items.clear();
        buffer_ops = 0;
    }
    
    typename std::vector<T>::iterator find(const T& val)
    {
        make_ready();
        typename std::vector<T>::size_type beg = 0;
        typename std::vector<T>::size_type end = size();
        
        while(beg < end)
        {
            typename std::vector<T>::size_type med = beg + (end - beg)/2;
            
            if(items[med] == val)
                return iterator(items.begin() + med);
            else if(items[med] > val)
                end = med;
            else
                beg = med;
        }
        
        return end();
    }
    
    typename std::vector<T>::size_type count(const T& val)
    {
        return find(val) != end();
    }
    
    typename std::vector<T>::iterator lower_bound(const T& val)
    {
        return std::lower_bound(items.begin(), items.end(), val);
    }
    
    typename std::vector<T>::iterator upper_bound(const T& val)
    {
        return std::upper_bound(items.begin(), items.end(), val);
    }
    
    typename std::vector<T>::iterator equal_range(const T& val)
    {
        return std::equal_range(items.begin(), items.end(), val);
    }
};

int main(int argc, char** argv)
{
    contiguous_set<int> s;
    for(int i = 0; i < 20; ++i)
        s.insert(i);
    
    for(auto it = s.begin(); it != s.end(); ++it)
        printf("%d\n", *it);
}