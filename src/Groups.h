#pragma once

#include "SgSystem.h"
#include "SgArrayList.h"
#include "SgHash.h"
#include "SgBoardColor.h"
#include "YException.h"
#include "ConstBoard.h"

#include <string>
#include <vector>

class Block;
class Board;
class SemiTable;
class SemiConnection;

//---------------------------------------------------------------------------

class Group
{
public:
    static const size_t MAX_BLOCKS = Y_MAX_CELL/2;
    static const cell_t NULL_GROUP = -1;

    typedef SgArrayList<cell_t, MAX_BLOCKS> BlockList;

    cell_t m_id;
    cell_t m_parent;
    cell_t m_block;
    cell_t m_left, m_right;
    uint32_t m_semi1, m_semi2;
    int m_border;
    BlockList m_blocks;
    MarkedCells m_carrier;
    MarkedCells m_con_carrier;

    Group();

    bool IsTopLevel() const
    { return m_parent == NULL_GROUP; }

    void ChangeChild(cell_t o, cell_t n)
    {
        if (m_left == o)
            m_left = n;
        else 
            m_right = n;
    }

    std::string BlocksToString() const;

    std::string CarrierToString() const;

    std::string ToString() const;

    bool ContainsBorder(cell_t border) const
    {
        return m_border & ConstBoard::ToBorderValue(border);
    }

    bool ContainsBlock(cell_t b) const
    {
        if (ConstBoard::IsEdge(b))
            return ContainsBorder(b);
        return m_blocks.Contains(b);
    }

    class BlockIterator
    {
    public:
        BlockIterator(const BlockList& bl, int start = 0)
            : m_index(start)
            , m_list(bl)
        { 
        }

        BlockIterator(const Group& g, int start = 0)
            : m_index(start)
            , m_list(g.m_blocks)
        {
            AppendEdges(g);
        }

        cell_t operator*() const
        { return m_list[m_index]; }
        
        int Index() const 
        { return m_index; }

        void operator++()
        { m_index++; }

        operator bool() const
        { return m_index < m_list.Length(); }

    private:
        int m_index;
        BlockList m_list;

        void AppendEdges(const Group& g)
        {
            if (g.m_border & ConstBoard::BORDER_WEST)
                m_list.PushBack(ConstBoard::WEST);
            if (g.m_border & ConstBoard::BORDER_EAST)
                m_list.PushBack(ConstBoard::EAST);
            if (g.m_border & ConstBoard::BORDER_SOUTH)
                m_list.PushBack(ConstBoard::SOUTH);
        }
    };
};

class Groups
{
public:
    explicit Groups(const SemiTable& semis);

    cell_t CreateSingleBlockGroup(const Block* block);

    cell_t SetGroupDataFromBlock(const Block* block, int id);
   
    void RestructureAfterMove(cell_t p, SgBlackWhite color, const Board& brd);

    void ProcessNewSemis(const Block* block,
                         std::vector<const SemiConnection*> s);

    const Group* GetGroup(cell_t p) const
    { return const_cast<Groups*>(this)->GetGroup(p); }

    Group* GetGroup(cell_t p);

private:
    // Need space for MAX_GROUPS * 2, and MAX_GROUPS < T/2, so this works.
    static const int MAX_GROUPS = Y_MAX_CELL;

    Group m_groupData[MAX_GROUPS];
    SgArrayList<int, MAX_GROUPS> m_rootGroups;
    SgArrayList<int, MAX_GROUPS> m_freelist;
    const SemiTable& m_semis;

    Group* GetGroupById(int gid)
    {
        return &m_groupData[gid];
    }

    void Detach(Group* g);

    void RecomputeFromChildren(Group* g);

    inline cell_t SiblingID(const Group* parent, cell_t child) const
    {
        return parent->m_left == child ? parent->m_right : parent->m_left;
    }

    bool CanMerge(const Group* ga, const Group* gb, 
                  SemiConnection const** x, SemiConnection const** y) const;

    bool CanMergeOnSemi(const Group* ga, const Group* gb,
                        const SemiConnection& x, 
                        SemiConnection const** outy) const;

    cell_t Merge(Group* g1, Group* g2, 
                 const SemiConnection& s1, const SemiConnection& s2);

    void RestructureAfterMove(Group* g, cell_t p, const Board& brd);

    void ComputeConnectionCarrier(Group* g);

    void FindGroupToMergeWith(Group* g, const Board& brd);
    
    cell_t ObtainID();
};

//---------------------------------------------------------------------------
