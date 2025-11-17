#pragma once

#include <cassert>
#include <cstddef>
#include <functional>
#include <limits>
#include <memory>
#include <vector>
#include <type_traits>
#include <iterator>
#include <utility>

namespace rb {

template <typename T>
class Node;

template <typename T>
class NodeBase {
public:
    enum class Color {
        RED,
        BLACK,
    };

    template <typename>
    friend class Tree;

    Color color() const { return color_; }
    void set_color(Color c) { color_ = c; }

    const NodeBase* left_child() const { return left_; }
    NodeBase* left_child() { return left_; }
    void set_left_child(NodeBase* l) { left_ = l; }

    const NodeBase* right_child() const { return right_; }
    NodeBase* right_child() { return right_; }
    void set_right_child(NodeBase* r) { right_ = r; }

    const NodeBase* parent() const { return parent_; }
    NodeBase* parent() { return parent_; }
    void set_parent(NodeBase* p) { parent_ = p; }

    std::size_t subtree_size() const { return subtree_size_; }
    void set_subtree_size(std::size_t size) { subtree_size_ = size; }

protected:
    NodeBase(Color c,
             NodeBase* l,
             NodeBase* r,
             NodeBase* p,
             std::size_t subtree_size = 0)
        : color_(c),
          left_(l),
          right_(r),
          parent_(p),
          subtree_size_(subtree_size) {}

    virtual ~NodeBase() {}

private:
    Color color_;
    NodeBase* left_;
    NodeBase* right_;
    NodeBase* parent_;
    std::size_t subtree_size_;
};

template <typename T>
class Node : public NodeBase<T> {
public:
    // Создаёт узел с копированием значения.
    Node(const T& v,
         typename NodeBase<T>::Color c = NodeBase<T>::Color::RED,
         NodeBase<T>* l = nullptr,
         NodeBase<T>* r = nullptr,
         NodeBase<T>* p = nullptr)
        : NodeBase<T>(c, l, r, p, 1), value_(v) {}

    // Создаёт узел с перемещением значения.
    Node(T&& v,
         typename NodeBase<T>::Color c = NodeBase<T>::Color::RED,
         NodeBase<T>* l = nullptr,
         NodeBase<T>* r = nullptr,
         NodeBase<T>* p = nullptr)
        : NodeBase<T>(c, l, r, p, 1), value_(std::move(v)) {}

    const T& value() const { return value_; }
    T& value() { return value_; }

    ~Node() override {}

private:
    T value_;
};

template <typename T>
bool compare_lower_bound(const T& first, const T& second) {  
    return first < second;
}

template <typename T>
bool compare_upper_bound(const T& first, const T& second) {
    return first <= second;
}

template <typename T>
class Tree {
public:
    enum class Direction { LEFT, RIGHT };
    class iterator;

    static_assert(std::is_copy_constructible_v<T>,
                  "rb::Tree<T> requires T to be copy-constructible");
    static_assert(std::is_move_constructible_v<T>,
                  "rb::Tree<T> requires T to be move-constructible");
    static_assert(std::is_copy_assignable_v<T>,
                  "rb::Tree<T> requires T to be copy-assignable");
    static_assert(std::is_move_assignable_v<T>,
                  "rb::Tree<T> requires T to be move-assignable");
    static_assert(std::is_convertible_v<
                      decltype(std::declval<const T&>() < std::declval<const T&>()),
                      bool>,
                  "rb::Tree<T> requires operator< to establish a strict ordering");

    class iterator {
    public:
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using reference = const T&;
        using pointer = const T*;
        using iterator_category = std::bidirectional_iterator_tag;

        iterator() = default;

        reference operator*() const {
            assert(current_ != nullptr);
            return owner_->as_node(current_)->value();
        }

        pointer operator->() const {
            return std::addressof(operator*());
        }

        iterator& operator++() {
            assert(current_ != nullptr);
            current_ = owner_->next(current_);
            return *this;
        }

        iterator operator++(int) {
            auto tmp = *this;
            ++(*this);
            return tmp;
        }

        iterator& operator--() {
            if (!current_) {
                current_ = owner_->maximum(owner_->root_);
            } else {
                current_ = owner_->prev(current_);
            }
            assert(current_ != nullptr);
            return *this;
        }

        iterator operator--(int) {
            auto tmp = *this;
            --(*this);
            return tmp;
        }

        bool operator==(const iterator& rhs) const {
            assert(owner_ == rhs.owner_);
            return current_ == rhs.current_;
        }

        bool operator!=(const iterator& rhs) const {
            return !(*this == rhs);
        }

    private:
        iterator(const Tree* owner, NodeBase<T>* current)
            : owner_(owner), current_(current) {}

        const Tree* owner_ = nullptr;
        NodeBase<T>* current_ = nullptr;

        friend class Tree;
    };

    iterator begin() {
        return root_ ? iterator(this, minimum(root_)) : end();
    }

    iterator end() {
        return iterator(this, nullptr);
    }

    iterator begin() const {
        return root_ ? iterator(this, minimum(root_)) : end();
    }

    iterator end() const {
        return iterator(this, nullptr);
    }

    // Инициализирует пустое дерево.
    Tree()
        : root_(nullptr) {}

    // Освобождает все узлы дерева.
    ~Tree() {
        clear(root_);
    }

    // Выполняет глубокое копирование.
    Tree(const Tree& other)
        : root_(nullptr) {
        root_ = clone_subtree(other.root_, nullptr);
    }

    // Перемещает данные из другого дерева.
    Tree(Tree&& other) noexcept
        : root_(std::exchange(other.root_, nullptr)) {}

    // Копирующее присваивание по идиоме copy-and-swap.
    Tree& operator=(const Tree& other) {
        if (this == &other) {
            return *this;
        }
        Tree tmp(other);
        *this = std::move(tmp);
        return *this;
    }

    // Перемещающее присваивание.
    Tree& operator=(Tree&& other) noexcept {
        if (this != &other) {
            Tree temp(std::move(other));
            std::swap(root_, temp.root_);
        }
        return *this;
    }

    // Проверяет, пусто ли дерево.
    bool empty() const { return root_ == nullptr; }

    // Вставляет значение, поддерживая баланс и статистики; false при дубликате.
    bool insert(const T& value) {
        auto result = locate(value);
        if (result.exists) {
            return false;
        }

        auto* parent = result.parent;
        auto* new_node = make_node(value,
                                   NodeBase<T>::Color::RED,
                                   nullptr,
                                   nullptr,
                                   parent);

        if (parent == nullptr) {
            root_ = new_node;
        } else if (result.go_left) {
            parent->set_left_child(new_node);
        } else {
            parent->set_right_child(new_node);
        }

        fix_insert_root(new_node);
        update_size_upwards(new_node);
        return true;
    }

    // Удаляет значение, восстанавливая баланс; возвращает false, если узла нет.
    bool erase(const T& value) {
        auto result = locate(value);
        if (!result.exists) {
            return false;
        }

        NodeBase<T>* z = result.parent;
        DetachResult detach = detach_node(z);
        delete z;

        if (detach.removed_color == node_color::BLACK) {
            erase_fixup(detach.fixup, detach.parent);
        }
        return true;
    }

    // Проверяет соблюдение инвариантов красно-чёрного дерева.
    bool is_valid() const {
        const NodeBase<T>* root = root_;

        if (root == nullptr) {
            return true;
        }

        if (root->color() != NodeBase<T>::Color::BLACK) {
            return false;
        }

        int black_height = 0;
        return validate_subtree(root, &black_height);
    }

    size_t distance(const T& first, const T& second) const {
        if (first > second) { 
            return 0;
        }

        size_t first_rank = rank_comp_bound(first, *compare_lower_bound<T>);
        size_t second_rank = rank_comp_bound(second, *compare_upper_bound<T>);

        assert(first_rank <= second_rank);
        return second_rank - first_rank;
    }

    // Возвращает позицию элемента в упорядоченном обходе.
    size_t distance_from_root(const T& value) const {
        auto location = locate(value);
        if (!location.exists) {
            return std::numeric_limits<size_t>::max();
        }
        return rank_comp_bound(value, compare_lower_bound<T>);
    }

    // Количество элементов в дереве.
    size_t size() const { return node_size(root_); }

    iterator lower_bound(const T& value) const {
        return iterator(this, lower_bound_node(value));
    }

    iterator upper_bound(const T& value) const {
        return iterator(this, upper_bound_node(value));
    }

    using cmp_t = std::function<bool(const T&, const T&)>; 
    size_t rank_comp_bound(const T& value, cmp_t cmp) const {;
        const NodeBase<T>* current = root_;
        size_t result = 0;

        while (!is_nil(current)) {
            const T& current_value = as_node(current)->value();
            if (cmp(current_value, value)) {
                result += node_size(current->left_child()) + 1;
                current = current->right_child();
            } else {
                current = current->left_child();
            }
        }

        return result;
    }


private:
    // Вспомогательная структура для locate.
    struct LocateResult {
        NodeBase<T>* parent;
        bool exists;
        bool go_left;
    };

    NodeBase<T>* root_;

    using node_color = typename NodeBase<T>::Color;

    struct DetachResult {
        NodeBase<T>* fixup;
        NodeBase<T>* parent;
        node_color removed_color;
    };

    bool is_nil(const NodeBase<T>* node) const { return node == nullptr; }

    // Приводит базовый указатель к типу Node<T>.
    Node<T>* as_node(NodeBase<T>* node) {
        return static_cast<Node<T>*>(node);
    }

    // Приводит базовый указатель к константному типу Node<T>.
    const Node<T>* as_node(const NodeBase<T>* node) const {
        return static_cast<const Node<T>*>(node);
    }

    // Возвращает цвет узла, считая nullptr чёрным.
    node_color color_of(const NodeBase<T>* node) const {
        return is_nil(node) ? NodeBase<T>::Color::BLACK : node->color();
    }

    node_color color_of(NodeBase<T>* node) const {
        return color_of(static_cast<const NodeBase<T>*>(node));
    }

    bool is_black(const NodeBase<T>* node) const {
        return color_of(node) == node_color::BLACK;
    }

    bool is_red(const NodeBase<T>* node) const {
        return color_of(node) == node_color::RED;
    }

    // Возвращает количество элементов в поддереве узла; 0 для nullptr.
    std::size_t node_size(const NodeBase<T>* node) const {
        return is_nil(node) ? 0 : node->subtree_size();
    }

    // Пересчитывает размер поддерева на основе детей.
    void recalc_size(NodeBase<T>* node) {
        if (is_nil(node)) {
            return;
        }
        const std::size_t left = node_size(node->left_child());
        const std::size_t right = node_size(node->right_child());
        node->set_subtree_size(left + right + 1);
    }

    // Поддерживает размеры всех предков узла актуальными.
    void update_size_upwards(NodeBase<T>* node) {
        while (!is_nil(node)) {
            recalc_size(node);
            node = node->parent();
        }
    }

    // Создаёт узел, заполняя указанные ссылки на детей и родителя.
    Node<T>* make_node(const T& value,
                       node_color color,
                       NodeBase<T>* left,
                       NodeBase<T>* right,
                       NodeBase<T>* parent) {
        auto* node = new Node<T>(value, color, left, right, parent);
        recalc_size(node);
        return node;
    }

    // Перегрузка для перемещающего создания узла.
    Node<T>* make_node(T&& value,
                       node_color color,
                       NodeBase<T>* left,
                       NodeBase<T>* right,
                       NodeBase<T>* parent) {
        auto* node =
            new Node<T>(std::move(value), color, left, right, parent);
        recalc_size(node);
        return node;
    }

    // Очищает поддерево, освобождая все узлы.
    void clear(NodeBase<T>* node) {
        if (node == nullptr) {
            return;
        }

        std::vector<NodeBase<T>*> stack;
        stack.push_back(node);

        while (!stack.empty()) {
            NodeBase<T>* current = stack.back();
            stack.pop_back();

            NodeBase<T>* left = current->left_child();
            if (left != nullptr) {
                stack.push_back(left);
            }

            NodeBase<T>* right = current->right_child();
            if (right != nullptr) {
                stack.push_back(right);
            }

            delete current; 
        }
    }

    // Находит место вставки или существующий узел.
    LocateResult locate(const T& value) const {
        NodeBase<T>* current = root_;
        NodeBase<T>* parent = nullptr;
        bool go_left = false;

        while (current != nullptr) {
            parent = current;
            const T& current_value = as_node(current)->value();
            if (value < current_value) {
                current = current->left_child();
                go_left = true;
            } else if (current_value < value) {
                current = current->right_child();
                go_left = false;
            } else {
                return {current, true, false};
            }
        }

        return {parent, false, go_left};
    }

    // Выполняет поворот вокруг узла в указанном направлении.
    void rotate(NodeBase<T>* node, Direction dir) {
        NodeBase<T>* pivot =
            (dir == Direction::LEFT) ? node->right_child() : node->left_child();
        assert(!is_nil(pivot));

        NodeBase<T>* pivot_parent = node->parent();
        if (pivot_parent == nullptr) {
            root_ = pivot;
            pivot->set_parent(nullptr);
        } else {
            pivot->set_parent(pivot_parent);
            if (node == pivot_parent->left_child()) {
                pivot_parent->set_left_child(pivot);
            } else {
                pivot_parent->set_right_child(pivot);
            }
        }

        if (dir == Direction::LEFT) {
            node->set_right_child(pivot->left_child());
            if (!is_nil(pivot->left_child())) {
                pivot->left_child()->set_parent(node);
            }
            pivot->set_left_child(node);
        } else {
            node->set_left_child(pivot->right_child());
            if (!is_nil(pivot->right_child())) {
                pivot->right_child()->set_parent(node);
            }
            pivot->set_right_child(node);
        }

        node->set_parent(pivot);
        recalc_size(node);
        recalc_size(pivot);
        update_size_upwards(pivot_parent);
    }

    void rotate_left(NodeBase<T>* node) {
        rotate(node, Direction::LEFT);
    }

    void rotate_right(NodeBase<T>* node) {
        rotate(node, Direction::RIGHT);
    }

    // Возвращает деда для текущего узла.
    NodeBase<T>* grandparent(NodeBase<T>* node) const {
        NodeBase<T>* parent = node->parent();
        return parent == nullptr ? nullptr : parent->parent();
    }

    // Возвращает дядю текущего узла.
    NodeBase<T>* uncle(NodeBase<T>* node) const {
        NodeBase<T>* grand = grandparent(node);
        if (grand == nullptr) {
            return nullptr;
        }

        if (node->parent() == grand->left_child()) {
            return grand->right_child();
        }
        return grand->left_child();
    }

    // Обрабатывает случай вставки корневого узла.
    void fix_insert_root(NodeBase<T>* node) {
        if (node->parent() == nullptr) {
            node->set_color(NodeBase<T>::Color::BLACK);
            root_ = node;
            return;
        }
        fix_insert_black_parent(node);
    }

    // Обрабатывает случай чёрного родителя.
    void fix_insert_black_parent(NodeBase<T>* node) {
        NodeBase<T>* parent = node->parent();
        if (parent == nullptr || parent->color() == NodeBase<T>::Color::BLACK) {
            return;
        }
        fix_insert_red_uncle(node);
    }

    // Обрабатывает случай красного дяди.
    void fix_insert_red_uncle(NodeBase<T>* node) {
        NodeBase<T>* u = uncle(node);
        if (u != nullptr && u->color() == NodeBase<T>::Color::RED) {
            node->parent()->set_color(NodeBase<T>::Color::BLACK);
            u->set_color(NodeBase<T>::Color::BLACK);
            NodeBase<T>* g = grandparent(node);
            if (g != nullptr) {
                g->set_color(NodeBase<T>::Color::RED);
                fix_insert_root(g);
            }
        } else {
            fix_insert_inner_child(node);
        }
    }

    // Выполняет промежуточные повороты.
    void fix_insert_inner_child(NodeBase<T>* node) {
        NodeBase<T>* parent = node->parent();
        NodeBase<T>* grand = grandparent(node);
        if (grand == nullptr || parent == nullptr) {
            return;
        }

        if (node == parent->right_child() && parent == grand->left_child()) {
            rotate_left(parent);
            node = node->left_child();
        } else if (node == parent->left_child() &&
                   parent == grand->right_child()) {
            rotate_right(parent);
            node = node->right_child();
        }

        finalize_insert_rebalance(node);
    }

    // Завершает восстановление баланса после вставки.
    void finalize_insert_rebalance(NodeBase<T>* node) {
        NodeBase<T>* parent = node->parent();
        NodeBase<T>* grand = grandparent(node);
        if (parent == nullptr || grand == nullptr) {
            return;
        }

        parent->set_color(NodeBase<T>::Color::BLACK);
        grand->set_color(NodeBase<T>::Color::RED);

        if (node == parent->left_child() && parent == grand->left_child()) {
            rotate_right(grand);
        } else if (node == parent->right_child() &&
                   parent == grand->right_child()) {
            rotate_left(grand);
        }
    }

    // Возвращает минимальный узел в поддереве.
    NodeBase<T>* minimum(NodeBase<T>* node) const {
        while (node != nullptr && node->left_child() != nullptr) {
            node = node->left_child();
        }
        return node;
    }

    // Возвращает максимальный узел в поддереве.
    NodeBase<T>* maximum(NodeBase<T>* node) const {
        while (node != nullptr && node->right_child() != nullptr) {
            node = node->right_child();
        }
        return node;
    }

    // Возвращает следующий узел.
    NodeBase<T>* next(NodeBase<T>* node) const {
        if (node == nullptr) {
            return nullptr;
        }
        if (node->right_child() != nullptr) {
            return minimum(node->right_child());
        }
        NodeBase<T>* parent = node->parent();
        while (parent != nullptr && node == parent->right_child()) {
            node = parent;
            parent = parent->parent();
        }
        return parent;
    }

    // Возвращает предыдущий узел.
    NodeBase<T>* prev(NodeBase<T>* node) const {
        if (node == nullptr) {
            return nullptr;
        }
        if (node->left_child() != nullptr) {
            return maximum(node->left_child());
        }
        NodeBase<T>* parent = node->parent();
        while (parent != nullptr && node == parent->left_child()) {
            node = parent;
            parent = parent->parent();
        }
        return parent;
    }

    template <typename Compare>
    NodeBase<T>* bound_node(const T& value, Compare go_left) const {
        NodeBase<T>* current = root_;
        NodeBase<T>* result = nullptr;

        while (current != nullptr) {
            const T& current_value = as_node(current)->value();
            if (go_left(value, current_value)) {
                result = current;
                current = current->left_child();
            } else {
                current = current->right_child();
            }
        }
        return result;
    }

    NodeBase<T>* lower_bound_node(const T& value) const {
        return bound_node(
            value,
            [](const T& target, const T& candidate) {
                return candidate >= target;
            });
    }

    NodeBase<T>* upper_bound_node(const T& value) const {
        return bound_node(
            value,
            [](const T& target, const T& candidate) {
                return candidate > target;
            });
    }

    // Обрабатывает удаление узла, у которого отсутствует один из детей.
    DetachResult detach_node_with_single_child(NodeBase<T>* z,
                                              NodeBase<T>* child) {
        NodeBase<T>* parent = z->parent();
        NodeBase<T>* replacement = child;
        transplant(z, replacement);

        NodeBase<T>* fixup_parent = (replacement != nullptr) ? replacement->parent()
                                                              : parent;
        return {replacement, fixup_parent, z->color()};
    }

    // Обрабатывает удаление узла при наличии обоих детей, используя преемника.
    DetachResult detach_node_with_successor(NodeBase<T>* z) {
        NodeBase<T>* successor = minimum(z->right_child());
        NodeBase<T>* x = successor->right_child();
        node_color removed_color = successor->color();
        bool successor_parent_is_z = (successor->parent() == z);
        NodeBase<T>* x_parent = nullptr;

        if (!successor_parent_is_z) {
            x_parent = successor->parent();
            transplant(successor, successor->right_child());
            successor->set_right_child(z->right_child());
            if (successor->right_child() != nullptr) {
                successor->right_child()->set_parent(successor);
            }
        } else {
            x_parent = successor;
            if (x != nullptr) {
                x->set_parent(successor);
            }
        }

        transplant(z, successor);
        successor->set_left_child(z->left_child());
        if (successor->left_child() != nullptr) {
            successor->left_child()->set_parent(successor);
        }
        successor->set_color(z->color());
        recalc_size(successor);
        update_size_upwards(successor);

        if (x != nullptr) {
            x_parent = x->parent();
        } else if (successor_parent_is_z) {
            x_parent = successor;
        }

        return {x, x_parent, removed_color};
    }

    // Переустанавливает узлы перед удалением, возвращая данные для исправления.
    DetachResult detach_node(NodeBase<T>* z) {
        if (is_nil(z->left_child())) {
            return detach_node_with_single_child(z, z->right_child());
        }

        if (is_nil(z->right_child())) {
            return detach_node_with_single_child(z, z->left_child());
        }

        return detach_node_with_successor(z);
    }

    // Заменяет одно поддерево другим и обновляет накопленные размеры.
    void transplant(NodeBase<T>* u, NodeBase<T>* v) {
        NodeBase<T>* parent = u->parent();

        if (parent == nullptr) {
            root_ = v;
        } else if (u == parent->left_child()) {
            parent->set_left_child(v);
        } else {
            parent->set_right_child(v);
        }
        if (v != nullptr) {
            v->set_parent(parent);
        }
        update_size_upwards(parent);
    }

    void paint(NodeBase<T>* node, node_color color) {
        if (!is_nil(node)) {
            node->set_color(color);
        }
    }

    // Исправляет двойной чёрный в направлении dir: разворачивает красного брата,
    // перекрашивает sibling и parent и выполняет нужные повороты.
    // Обеспечивает чёрного брата: если sibling красный, переворачиваем узлы.
    void ensure_black_sibling(NodeBase<T>*& parent,
                              NodeBase<T>*& sibling,
                              Direction dir) {
        if (is_red(sibling)) {
            paint(sibling, node_color::BLACK);
            paint(parent, node_color::RED);
            rotate(parent, dir);
            sibling = (dir == Direction::LEFT) ? parent->right_child()
                                               : parent->left_child();
        }
    }

    // Возвращает внутреннего и внешнего потомков брата относительно направления.
    void fetch_sibling_children(NodeBase<T>* sibling,
                                Direction dir,
                                NodeBase<T>** inner,
                                NodeBase<T>** outer) {
        if (sibling == nullptr) {
            *inner = *outer = nullptr;
            return;
        }
        if (dir == Direction::LEFT) {
            *inner = sibling->left_child();
            *outer = sibling->right_child();
        } else {
            *inner = sibling->right_child();
            *outer = sibling->left_child();
        }
    }

    // Выполняет финальный поворот и перекраску после исправления двойного чёрного.
    void apply_outer_rotation(NodeBase<T>*& node,
                              NodeBase<T>*& parent,
                              NodeBase<T>* sibling,
                              NodeBase<T>* sibling_outer,
                              Direction dir) {
        paint(sibling, parent ? parent->color() : node_color::BLACK);
        paint(parent, node_color::BLACK);
        paint(sibling_outer, node_color::BLACK);
        rotate(parent, dir);
        node = root_;
        parent = nullptr;
    }

    void erase_fixup_direction(NodeBase<T>*& node,
                               NodeBase<T>*& parent,
                               Direction dir) {
        if (parent == nullptr) {
            node = root_;
            return;
        }

        NodeBase<T>* sibling =
            (dir == Direction::LEFT) ? parent->right_child()
                                     : parent->left_child();

        ensure_black_sibling(parent, sibling, dir);

        NodeBase<T>* sibling_inner = nullptr;
        NodeBase<T>* sibling_outer = nullptr;
        fetch_sibling_children(sibling, dir, &sibling_inner, &sibling_outer);

        if (is_black(sibling_inner) && is_black(sibling_outer)) {
            paint(sibling, node_color::RED);
            node = parent;
            parent = parent->parent();
            return;
        }

        if (is_black(sibling_outer)) {
            paint(sibling_inner, node_color::BLACK);
            paint(sibling, node_color::RED);
            rotate(sibling,
                   (dir == Direction::LEFT) ? Direction::RIGHT
                                             : Direction::LEFT);
            sibling =
                (dir == Direction::LEFT) ? parent->right_child()
                                         : parent->left_child();
            fetch_sibling_children(sibling, dir, &sibling_inner, &sibling_outer);
        }

        apply_outer_rotation(node, parent, sibling, sibling_outer, dir);
    }

    void erase_fixup(NodeBase<T>* node, NodeBase<T>* parent) {
        while (node != root_ && is_black(node)) {
            if (parent != nullptr && node == parent->left_child()) {
                erase_fixup_direction(node, parent, Direction::LEFT);
            } else {
                erase_fixup_direction(node, parent, Direction::RIGHT);
            }
        }

        paint(node, node_color::BLACK);
    }

    // Рекурсивно проверяет инварианты поддерева.
    bool validate_subtree(const NodeBase<T>* node,
                          int* black_height_out) const {
        if (node == nullptr) {
            *black_height_out = 1;
            return true;
        }

        const NodeBase<T>* left = node->left_child();
        const NodeBase<T>* right = node->right_child();

        if (node->color() == NodeBase<T>::Color::RED) {
            if ((left != nullptr && left->color() == NodeBase<T>::Color::RED) ||
                (right != nullptr && right->color() == NodeBase<T>::Color::RED)) {
                return false;
            }
        }

        int left_black_height = 0;
        if (!validate_subtree(left, &left_black_height)) {
            return false;
        }

        int right_black_height = 0;
        if (!validate_subtree(right, &right_black_height)) {
            return false;
        }

        if (left_black_height != right_black_height) {
            return false;
        }

        *black_height_out =
            left_black_height +
            (node->color() == NodeBase<T>::Color::BLACK ? 1 : 0);
        return true;
    }

    // Клонирует поддерево, переназначая родительские указатели.
    NodeBase<T>* clone_subtree(const NodeBase<T>* node,
                               NodeBase<T>* parent) {
        if (node == nullptr) {
            return nullptr;
        }

        auto* new_node = make_node(as_node(node)->value(),
                                   node->color(),
                                   nullptr,
                                   nullptr,
                                   parent);

        NodeBase<T>* left_child = clone_subtree(node->left_child(), new_node);
        new_node->set_left_child(left_child);
        if (left_child != nullptr) {
            left_child->set_parent(new_node);
        }

        NodeBase<T>* right_child = clone_subtree(node->right_child(), new_node);
        new_node->set_right_child(right_child);
        if (right_child != nullptr) {
            right_child->set_parent(new_node);
        }

        recalc_size(new_node);
        return new_node;
    }

    // Возвращает порядковый номер узла в симметричном обходе.
    size_t order_statistics(const NodeBase<T>* node) const {
        size_t count = 0;
        const T& target = as_node(node)->value();
        const NodeBase<T>* current = root_;
        while (!is_nil(current)) {
            const T& current_value = as_node(current)->value();
            if (target < current_value) {
                current = current->left_child();
            } else if (current_value < target) {
                count += subtree_size(current->left_child()) + 1;
                current = current->right_child();
            } else {
                count += subtree_size(current->left_child());
                break;
            }
        }
        return count;
    }

    size_t subtree_size(const NodeBase<T>* node) const {
        if (is_nil(node)) {
            return 0;
        }
        return node_size(node);
    }
};


} // namespace rb
