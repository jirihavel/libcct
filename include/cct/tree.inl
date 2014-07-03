template<typename C, typename L>
typename Tree<C,L>::size_type
Tree<C,L>::countDegenerateComponents() const
{
    size_type count = 0;
    forEach(
        [&](Component const & c)
        {
            if(c.isDegenerate())
                ++count;
        },
        utils::fp::ignore(), utils::fp::ignore()
    );
    return count;
}

template<typename C, typename L>
typename Tree<C,L>::size_type
Tree<C,L>::calculateHeight() const
{
    size_type height = 0;
    for(size_type i = 0; i < leaf_count; ++i)
    {
        height = std::max(height, leaf(i)->calculateHeight());
    }
    return height;
}

// Pretty printing

template<typename C, typename L>
void Tree<C,L>::prettyPrint(std::ostream & out, Leaf const & leaf)
{
    out << '(' << leafId(leaf) << ':' << leaf << ")\n";
}

template<typename C, typename L>
void Tree<C,L>::prettyPrint(std::ostream & out, Node const & node, std::string indent, bool last)
{
    out << indent;
    if(last)
    {
        out << "\\-";
        indent += "  ";
    }
    else
    {
        out << "|-";
        indent += "| ";
    }

    if(isNodeLeaf(node))
    {
        prettyPrint(out, static_cast<Leaf const &>(node));
    }
    else
    {
        Component const & comp = static_cast<Component const &>(node);
        out << '[' << comp << "]\n";
        if(!comp.empty())
        {
            typename Component::const_iterator i = comp.begin();
            do
            {
                typename Component::const_iterator j = i++;
                last = (i == comp.end());
                prettyPrint(out, *j, indent, last);
            } while(!last);
        }
    }
}

template<typename C, typename L>
void Tree<C,L>::prettyPrint(std::ostream & out)
{
    out << "roots=" << roots.size() << ", inner=" << component_count << ", leaves=" << leaf_count << std::endl;
    typename Roots::const_iterator i = roots.begin();
    if(i != roots.end())
    {
        bool last = false;
        do
        {
            typename Roots::const_iterator j = i++;
            last = (i == roots.end());
            prettyPrint(out, *j, "", last);
        } while(!last);
    }
}
