#pragma once

#include <cassert>
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

    // Возвращает цвет узла.
    Color color() const { return color_; }
    // Устанавливает цвет узла.
    void set_color(Color c) { color_ = c; }

    // Возвращает указатель на левого потомка.
    const NodeBase* left_child() const { return left_; }
    // Возвращает изменяемый указатель на левого потомка.
    NodeBase* left_child() { return left_; }
    // Устанавливает левого потомка.
    void set_left_child(NodeBase* l) { left_ = l; }

    // Возвращает указатель на правого потомка.
    const NodeBase* right_child() const { return right_; }
    // Возвращает изменяемый указатель на правого потомка.
    NodeBase* right_child() { return right_; }
    // Устанавливает правого потомка.
    void set_right_child(NodeBase* r) { right_ = r; }

    // Возвращает родителя.
    const NodeBase* parent() const { return parent_; }
    // Возвращает изменяемого родителя.
    NodeBase* parent() { return parent_; }
    // Устанавливает родителя.
    void set_parent(NodeBase* p) { parent_ = p; }

protected:
    // Создаёт базовый узел с указанными связями.
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

    // Возвращает значение узла.
    const T& value() const { return value_; }
    // Возвращает изменяемое значение узла.
    T& value() { return value_; }

private:
    T value_;
};

template <typename T>
class Tree {
public:
    // Инициализирует пустое дерево со сторожевым узлом.
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

        NodeBase<T>* z = result.parent;
        NodeBase<T>* y = z;
        auto y_original_color = y->color();
        NodeBase<T>* x = &nil_;

        if (is_nil(z->left_child())) {
            x = z->right_child();
            transplant(z, z->right_child());
        } else if (is_nil(z->right_child())) {
            x = z->left_child();
            transplant(z, z->left_child());
        } else {
            y = minimum(z->right_child());
            y_original_color = y->color();
            x = y->right_child();
            if (y->parent() == z) {
                x->set_parent(y);
            } else {
                transplant(y, y->right_child());
                y->set_right_child(z->right_child());
                if (!is_nil(y->right_child())) {
                    y->right_child()->set_parent(y);
                }
            }

            transplant(z, y);
            y->set_left_child(z->left_child());
            if (!is_nil(y->left_child())) {
                y->left_child()->set_parent(y);
            }
            y->set_color(z->color());
        }

        delete as_node(z);

        if (y_original_color == NodeBase<T>::Color::BLACK) {
            erase_fixup(x);
        }

        if (is_nil(root_)) {
            root_ = &nil_;
        } else {
            root_->set_parent(&nil_);
            root_->set_color(NodeBase<T>::Color::BLACK);
        }

        return true;
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

    // Проверяет, является ли указатель сторожевым узлом.
    bool is_nil(const NodeBase<T>* node) const { return node == &nil_; }

    // Приводит базовый указатель к типу Node<T>.
    Node<T>* as_node(NodeBase<T>* node) {
        return static_cast<Node<T>*>(node);
    }

    // Приводит базовый указатель к константному типу Node<T>.
    const Node<T>* as_node(const NodeBase<T>* node) const {
        return static_cast<const Node<T>*>(node);
    }

    // Возвращает цвет узла с учётом sentinel.
    typename NodeBase<T>::Color color_of(NodeBase<T>* node) const {
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
