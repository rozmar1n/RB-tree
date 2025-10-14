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
    Node(const T& v,
         typename NodeBase<T>::Color c = NodeBase<T>::Color::RED,
         NodeBase<T>* l = nullptr,
         NodeBase<T>* r = nullptr,
         NodeBase<T>* p = nullptr)
        : NodeBase<T>(c, l, r, p), value_(v) {}

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
    Tree()
        : nil_(NodeBase<T>::Color::BLACK, &nil_, &nil_, &nil_),
          root_(&nil_) {}

    ~Tree() { clear(root_); }

    Tree(const Tree&) = delete;
    Tree& operator=(const Tree&) = delete;
    Tree(Tree&&) = delete;
    Tree& operator=(Tree&&) = delete;

    //TODO: реализовать новые методы копирования, удаления и т. д.


    bool empty() const { return root_ == &nil_; }

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

    bool erase(const T&) {
        //TODO: реализовать удаление узла 
        return false;
    }

private:
    struct LocateResult {
        NodeBase<T>* parent;
        bool exists;
        bool go_left;
    };

    NodeBase<T> nil_;
    NodeBase<T>* root_;

    bool is_nil(const NodeBase<T>* node) const { return node == &nil_; }

    Node<T>* as_node(NodeBase<T>* node) {
        return static_cast<Node<T>*>(node);
    }

    const Node<T>* as_node(const NodeBase<T>* node) const {
        return static_cast<const Node<T>*>(node);
    }

    void clear(NodeBase<T>* node) {
        if (is_nil(node)) {
            return;
        }
        clear(node->left_child());
        clear(node->right_child());
        delete as_node(node);
    }

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

    NodeBase<T>* grandparent(NodeBase<T>* node) const {
        NodeBase<T>* parent = node->parent();
        if (is_nil(parent)) {
            return &nil_;
        }
        return parent->parent();
    }

    NodeBase<T>* uncle(NodeBase<T>* node) const {
        NodeBase<T>* grand = grandparent(node);
        if (is_nil(grand)) {
            return &nil_;
        }

        if (node->parent() == grand->left_child()) {
            return grand->right_child();
        }
        return grand->left_child();
    }

    void insert_case1(NodeBase<T>* node) {
        if (node->parent() == &nil_) {
            node->set_color(NodeBase<T>::Color::BLACK);
            root_ = node;
            return;
        }
        insert_case2(node);
    }

    void insert_case2(NodeBase<T>* node) {
        if (node->parent()->color() == NodeBase<T>::Color::BLACK) {
            return;
        }
        insert_case3(node);
    }

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
};

} // namespace rb
