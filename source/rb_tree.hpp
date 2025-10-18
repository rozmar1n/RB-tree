#pragma once

#include <cassert>
#include <cstddef>
#include <limits>
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

protected:
    NodeBase(Color c, NodeBase* l, NodeBase* r, NodeBase* p)
        : color_(c), left_(l), right_(r), parent_(p) {}

private:
    Color color_;
    NodeBase* left_;
    NodeBase* right_;
    NodeBase* parent_;
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
        : NodeBase<T>(c, l, r, p), value_(v) {}

    // Создаёт узел с перемещением значения.
    Node(T&& v,
         typename NodeBase<T>::Color c = NodeBase<T>::Color::RED,
         NodeBase<T>* l = nullptr,
         NodeBase<T>* r = nullptr,
         NodeBase<T>* p = nullptr)
        : NodeBase<T>(c, l, r, p), value_(std::move(v)) {}

    const T& value() const { return value_; }
    T& value() { return value_; }

private:
    T value_;
};

template <typename T>
class Tree {
public:
    // Инициализирует пустое дерево со nil_ узлом.
    Tree()
        : nil_(NodeBase<T>::Color::BLACK, &nil_, &nil_, &nil_),
          root_(&nil_) {
        reset_nil_links();
    }

    // Освобождает все узлы дерева.
    ~Tree() {
        clear(root_);
        root_ = &nil_;
    }

    // Выполняет глубокое копирование.
    Tree(const Tree& other)
        : nil_(NodeBase<T>::Color::BLACK, &nil_, &nil_, &nil_),
          root_(&nil_) {
        reset_nil_links();
        root_ = clone_subtree(other.root_, &nil_, &other.nil_);
    }

    // Перемещает данные из другого дерева.
    Tree(Tree&& other) noexcept
        : nil_(NodeBase<T>::Color::BLACK, &nil_, &nil_, &nil_),
          root_(&nil_) {
        reset_nil_links();
        if (!other.empty()) {
            root_ = other.root_;
            rebind_nil(root_, &other.nil_, &nil_);
            other.root_ = &other.nil_;
            other.reset_nil_links();
        }
    }

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
        if (this == &other) {
            return *this;
        }
        clear(root_);
        root_ = &nil_;
        reset_nil_links();

        if (!other.empty()) {
            root_ = other.root_;
            rebind_nil(root_, &other.nil_, &nil_);
            other.root_ = &other.nil_;
            other.reset_nil_links();
        }
        return *this;
    }

    // Проверяет, пусто ли дерево.
    bool empty() const { return root_ == &nil_; }

    // Вставляет значение, возвращает false при наличии дубликата.
    bool insert(const T& value) {
        auto result = locate(value);
        if (result.exists) {
            return false;
        }

        auto* parent = result.parent;
        auto* new_node = new Node<T>(value,
                                     NodeBase<T>::Color::RED,
                                     &nil_,
                                     &nil_,
                                     parent);

        if (parent == &nil_) {
            root_ = new_node;
        } else if (result.go_left) {
            parent->set_left_child(new_node);
        } else {
            parent->set_right_child(new_node);
        }

        insert_case1(new_node);
        return true;
    }

    // Удаляет значение из дерева, если оно присутствует.
    bool erase(const T& value) {
        auto result = locate(value);
        if (!result.exists) {
            return false;
        }

        NodeBase<T>* target = result.parent;
        EraseContext ctx = detach_erase_target(target);

        delete as_node(target);

        finalize_erase(ctx);
        return true;
    }

    // Проверяет соблюдение инвариантов красно-чёрного дерева.
    bool is_valid() const {
        const NodeBase<T>* root = root_;
        const NodeBase<T>* nil = &nil_;

        if (root == nil) {
            return root->color() == NodeBase<T>::Color::BLACK;
        }

        if (root->color() != NodeBase<T>::Color::BLACK) {
            return false;
        }

        int black_height = 0;
        return validate_subtree(root, nil, &black_height);
    }

    // Возвращает количество элементов, расположенныхв диапазоне [first, second).
    size_t distance(const T& first, const T& second) const {
        auto first_location = locate(first);
        auto second_location = locate(second);

        if (!first_location.exists || !second_location.exists) {
            return std::numeric_limits<size_t>::max();
        }

        const size_t first_rank = rank_lower_bound(first);
        const size_t second_rank = rank_lower_bound(second);

        if (first_rank >= second_rank) {
            return 0;
        }
        return second_rank - first_rank;
    }

    // Возвращает позицию элемента в упорядоченном обходе.
    size_t distance_from_root(const T& value) const {
        auto location = locate(value);
        if (!location.exists) {
            return std::numeric_limits<size_t>::max();
        }
        return rank_lower_bound(value);
    }

    // Количество элементов в дереве.
    size_t size() const { return subtree_size(root_); }

    // Индекс первого элемента >= value; size() если такого нет.
    size_t rank_lower_bound(const T& value) const {
        const NodeBase<T>* current = root_;
        const NodeBase<T>* candidate = &nil_;

        while (!is_nil(current)) {
            const T& current_value = as_node(current)->value();
            if (current_value < value) {
                current = current->right_child();
            } else {
                candidate = current;
                current = current->left_child();
            }
        }

        if (candidate == &nil_) {
            return size();
        }
        return order_statistics(candidate);
    }

    // Индекс первого элемента > value; size() если такого нет.
    size_t rank_upper_bound(const T& value) const {
        const NodeBase<T>* current = root_;
        const NodeBase<T>* candidate = &nil_;

        while (!is_nil(current)) {
            const T& current_value = as_node(current)->value();
            if (value < current_value) {
                candidate = current;
                current = current->left_child();
            } else {
                current = current->right_child();
            }
        }

        if (candidate == &nil_) {
            return size();
        }
        return order_statistics(candidate);
    }

private:
    // Вспомогательная структура для locate.
    struct LocateResult {
        NodeBase<T>* parent;
        bool exists;
        bool go_left;
    };

    NodeBase<T> nil_;
    NodeBase<T>* root_;

    using node_color = typename NodeBase<T>::Color;

    struct EraseContext {
        NodeBase<T>* fixup_node;
        node_color removed_color;
    };

    bool is_nil(const NodeBase<T>* node) const { return node == &nil_; }

    // Приводит базовый указатель к типу Node<T>.
    Node<T>* as_node(NodeBase<T>* node) {
        return static_cast<Node<T>*>(node);
    }

    // Приводит базовый указатель к константному типу Node<T>.
    const Node<T>* as_node(const NodeBase<T>* node) const {
        return static_cast<const Node<T>*>(node);
    }

    // Возвращает цвет узла с учётом nil_.
    node_color color_of(NodeBase<T>* node) const {
        return is_nil(node) ? NodeBase<T>::Color::BLACK : node->color();
    }

    // Очищает поддерево, освобождая все узлы.
    void clear(NodeBase<T>* node) {
        if (is_nil(node)) {
            return;
        }
        clear(node->left_child());
        clear(node->right_child());
        delete as_node(node);
    }

    // Находит место вставки или существующий узел.
    LocateResult locate(const T& value) const {
        NodeBase<T>* current = root_;
        NodeBase<T>* parent = const_cast<NodeBase<T>*>(&nil_);
        bool go_left = false;

        while (!is_nil(current)) {
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

    // Выполняет левый поворот вокруг узла.
    void rotate_left(NodeBase<T>* node) {
        NodeBase<T>* pivot = node->right_child();
        assert(!is_nil(pivot));

        pivot->set_parent(node->parent());
        if (node->parent() == &nil_) {
            root_ = pivot;
            pivot->set_parent(&nil_);
        } else if (node == node->parent()->left_child()) {
            node->parent()->set_left_child(pivot);
        } else {
            node->parent()->set_right_child(pivot);
        }

        node->set_right_child(pivot->left_child());
        if (!is_nil(pivot->left_child())) {
            pivot->left_child()->set_parent(node);
        }

        pivot->set_left_child(node);
        node->set_parent(pivot);
    }

    // Выполняет правый поворот вокруг узла.
    void rotate_right(NodeBase<T>* node) {
        NodeBase<T>* pivot = node->left_child();
        assert(!is_nil(pivot));

        pivot->set_parent(node->parent());
        if (node->parent() == &nil_) {
            root_ = pivot;
            pivot->set_parent(&nil_);
        } else if (node == node->parent()->left_child()) {
            node->parent()->set_left_child(pivot);
        } else {
            node->parent()->set_right_child(pivot);
        }

        node->set_left_child(pivot->right_child());
        if (!is_nil(pivot->right_child())) {
            pivot->right_child()->set_parent(node);
        }

        pivot->set_right_child(node);
        node->set_parent(pivot);
    }

    // Возвращает деда для текущего узла.
    NodeBase<T>* grandparent(NodeBase<T>* node) const {
        NodeBase<T>* parent = node->parent();
        if (is_nil(parent)) {
            return const_cast<NodeBase<T>*>(&nil_);
        }
        return parent->parent();
    }

    // Возвращает дядю текущего узла.
    NodeBase<T>* uncle(NodeBase<T>* node) const {
        NodeBase<T>* grand = grandparent(node);
        if (is_nil(grand)) {
            return const_cast<NodeBase<T>*>(&nil_);
        }

        if (node->parent() == grand->left_child()) {
            return grand->right_child();
        }
        return grand->left_child();
    }

    // Обрабатывает случай вставки корневого узла.
    void insert_case1(NodeBase<T>* node) {
        if (node->parent() == &nil_) {
            node->set_color(NodeBase<T>::Color::BLACK);
            root_ = node;
            return;
        }
        insert_case2(node);
    }

    // Обрабатывает случай чёрного родителя.
    void insert_case2(NodeBase<T>* node) {
        if (node->parent()->color() == NodeBase<T>::Color::BLACK) {
            return;
        }
        insert_case3(node);
    }

    // Обрабатывает случай красного дяди.
    void insert_case3(NodeBase<T>* node) {
        NodeBase<T>* u = uncle(node);
        if (!is_nil(u) && u->color() == NodeBase<T>::Color::RED) {
            node->parent()->set_color(NodeBase<T>::Color::BLACK);
            u->set_color(NodeBase<T>::Color::BLACK);
            NodeBase<T>* g = grandparent(node);
            g->set_color(NodeBase<T>::Color::RED);
            insert_case1(g);
        } else {
            insert_case4(node);
        }
    }

    // Выполняет промежуточные повороты.
    void insert_case4(NodeBase<T>* node) {
        NodeBase<T>* parent = node->parent();
        NodeBase<T>* grand = grandparent(node);

        if (node == parent->right_child() && parent == grand->left_child()) {
            rotate_left(parent);
            node = node->left_child();
        } else if (node == parent->left_child() &&
                   parent == grand->right_child()) {
            rotate_right(parent);
            node = node->right_child();
        }

        insert_case5(node);
    }

    // Завершает восстановление баланса после вставки.
    void insert_case5(NodeBase<T>* node) {
        NodeBase<T>* parent = node->parent();
        NodeBase<T>* grand = grandparent(node);

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
        while (!is_nil(node->left_child())) {
            node = node->left_child();
        }
        return node;
    }

    // Заменяет одно поддерево другим.
    void transplant(NodeBase<T>* u, NodeBase<T>* v) {
        if (u->parent() == &nil_) {
            root_ = v;
        } else if (u == u->parent()->left_child()) {
            u->parent()->set_left_child(v);
        } else {
            u->parent()->set_right_child(v);
        }
        v->set_parent(u->parent());
    }

    // Удаляет узел и готовит данные для восстановления баланса.
    EraseContext detach_erase_target(NodeBase<T>* node) {
        NodeBase<T>* successor = node;
        node_color removed_color = successor->color();
        NodeBase<T>* fixup_node = &nil_;

        if (is_nil(node->left_child())) {
            fixup_node = node->right_child();
            transplant(node, node->right_child());
        } else if (is_nil(node->right_child())) {
            fixup_node = node->left_child();
            transplant(node, node->left_child());
        } else {
            successor = minimum(node->right_child());
            removed_color = successor->color();
            fixup_node = successor->right_child();
            if (successor->parent() == node) {
                fixup_node->set_parent(successor);
            } else {
                transplant(successor, successor->right_child());
                successor->set_right_child(node->right_child());
                if (!is_nil(successor->right_child())) {
                    successor->right_child()->set_parent(successor);
                }
            }

            transplant(node, successor);
            successor->set_left_child(node->left_child());
            if (!is_nil(successor->left_child())) {
                successor->left_child()->set_parent(successor);
            }
            successor->set_color(node->color());
        }

        return EraseContext{fixup_node, removed_color};
    }

    // Завершает процедуру удаления узла
    void finalize_erase(const EraseContext& ctx) {
        if (ctx.removed_color == NodeBase<T>::Color::BLACK) {
            erase_fixup(ctx.fixup_node);
        }

        if (is_nil(root_)) {
            root_ = &nil_;
        } else {
            root_->set_parent(&nil_);
            root_->set_color(NodeBase<T>::Color::BLACK);
        }
    }
    
    // Обрабатывает случаи восстановления, когда текущий узел — левый ребёнок.
    NodeBase<T>* erase_fixup_left(NodeBase<T>* node) {
        NodeBase<T>* sibling = node->parent()->right_child();
        if (color_of(sibling) == NodeBase<T>::Color::RED) {
            if (!is_nil(sibling)) {
                sibling->set_color(NodeBase<T>::Color::BLACK);
            }
            node->parent()->set_color(NodeBase<T>::Color::RED);
            rotate_left(node->parent());
            sibling = node->parent()->right_child();
        }

        bool sibling_children_black =
            color_of(sibling->left_child()) == NodeBase<T>::Color::BLACK &&
            color_of(sibling->right_child()) == NodeBase<T>::Color::BLACK;
        if (sibling_children_black) {
            if (!is_nil(sibling)) {
                sibling->set_color(NodeBase<T>::Color::RED);
            }
            return node->parent();
        }

        if (color_of(sibling->right_child()) == NodeBase<T>::Color::BLACK) {
            if (!is_nil(sibling->left_child())) {
                sibling->left_child()->set_color(NodeBase<T>::Color::BLACK);
            }
            if (!is_nil(sibling)) {
                sibling->set_color(NodeBase<T>::Color::RED);
            }
            rotate_right(sibling);
            sibling = node->parent()->right_child();
        }

        if (!is_nil(sibling)) {
            sibling->set_color(node->parent()->color());
        }
        node->parent()->set_color(NodeBase<T>::Color::BLACK);
        if (!is_nil(sibling->right_child())) {
            sibling->right_child()->set_color(NodeBase<T>::Color::BLACK);
        }
        rotate_left(node->parent());
        return root_;
    }

    // Обрабатывает симметричные случаи, когда узел — правый ребёнок.
    NodeBase<T>* erase_fixup_right(NodeBase<T>* node) {
        NodeBase<T>* sibling = node->parent()->left_child();
        if (color_of(sibling) == NodeBase<T>::Color::RED) {
            if (!is_nil(sibling)) {
                sibling->set_color(NodeBase<T>::Color::BLACK);
            }
            node->parent()->set_color(NodeBase<T>::Color::RED);
            rotate_right(node->parent());
            sibling = node->parent()->left_child();
        }

        bool sibling_children_black =
            color_of(sibling->right_child()) == NodeBase<T>::Color::BLACK &&
            color_of(sibling->left_child()) == NodeBase<T>::Color::BLACK;
        if (sibling_children_black) {
            if (!is_nil(sibling)) {
                sibling->set_color(NodeBase<T>::Color::RED);
            }
            return node->parent();
        }

        if (color_of(sibling->left_child()) == NodeBase<T>::Color::BLACK) {
            if (!is_nil(sibling->right_child())) {
                sibling->right_child()->set_color(NodeBase<T>::Color::BLACK);
            }
            if (!is_nil(sibling)) {
                sibling->set_color(NodeBase<T>::Color::RED);
            }
            rotate_left(sibling);
            sibling = node->parent()->left_child();
        }

        if (!is_nil(sibling)) {
            sibling->set_color(node->parent()->color());
        }
        node->parent()->set_color(NodeBase<T>::Color::BLACK);
        if (!is_nil(sibling->left_child())) {
            sibling->left_child()->set_color(NodeBase<T>::Color::BLACK);
        }
        rotate_right(node->parent());
        return root_;
    }

    // Восстанавливает свойства дерева после удаления.
    void erase_fixup(NodeBase<T>* node) {
        while (node != root_ &&
               color_of(node) == NodeBase<T>::Color::BLACK) {
            if (node == node->parent()->left_child()) {
                node = erase_fixup_left(node);
            } else {
                node = erase_fixup_right(node);
            }
        }
        node->set_color(NodeBase<T>::Color::BLACK);
    }

    //рекурсивно проверяет инваринты поддерева
    bool validate_subtree(const NodeBase<T>* node,
                          const NodeBase<T>* nil,
                          int* black_height_out) const {
        if (node == nil) {
            *black_height_out = 1;
            return true;
        }

        const NodeBase<T>* left = node->left_child();
        const NodeBase<T>* right = node->right_child();

        if (node->color() == NodeBase<T>::Color::RED) {
            if ((left != nil && left->color() == NodeBase<T>::Color::RED) ||
                (right != nil && right->color() == NodeBase<T>::Color::RED)) {
                return false;
            }
        }

        int left_black_height = 0;
        if (!validate_subtree(left, nil, &left_black_height)) {
            return false;
        }

        int right_black_height = 0;
        if (!validate_subtree(right, nil, &right_black_height)) {
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

    // Клонирует поддерево, используя другой sentinel.
    NodeBase<T>* clone_subtree(const NodeBase<T>* node,
                               NodeBase<T>* parent,
                               const NodeBase<T>* other_nil) {
        if (node == other_nil) {
            return &nil_;
        }

        auto* new_node = new Node<T>(as_node(node)->value(),
                                     node->color(),
                                     &nil_,
                                     &nil_,
                                     parent);

        NodeBase<T>* left_child =
            clone_subtree(node->left_child(), new_node, other_nil);
        new_node->set_left_child(left_child);

        NodeBase<T>* right_child =
            clone_subtree(node->right_child(), new_node, other_nil);
        new_node->set_right_child(right_child);

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
        return 1 + subtree_size(node->left_child()) +
               subtree_size(node->right_child());
    }

    // Переназначает ссылку на sentinel после перемещения.
    void rebind_nil(NodeBase<T>* node,
                    NodeBase<T>* old_nil,
                    NodeBase<T>* new_nil) {
        if (node == old_nil) {
            return;
        }

        if (node->parent() == old_nil) {
            node->set_parent(new_nil);
        }

        NodeBase<T>* left = node->left_child();
        if (left == old_nil) {
            node->set_left_child(new_nil);
        } else {
            rebind_nil(left, old_nil, new_nil);
        }

        NodeBase<T>* right = node->right_child();
        if (right == old_nil) {
            node->set_right_child(new_nil);
        } else {
            rebind_nil(right, old_nil, new_nil);
        }
    }

    // Возвращает sentinel в исходное состояние.
    void reset_nil_links() {
        nil_.set_left_child(&nil_);
        nil_.set_right_child(&nil_);
        nil_.set_parent(&nil_);
        nil_.set_color(NodeBase<T>::Color::BLACK);
    }
};

} // namespace rb
