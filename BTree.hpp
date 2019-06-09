#include "utility.hpp"
#include <functional>
#include <cstddef>
#include "exception.hpp"
#include <cstdio>
namespace sjtu {
    //B+树索引存储地址
    constexpr char b+t_ad[128] = "mybptree.sjtu";
    template <class K, class Val, class Cmp = std::less<K> >
    class BTree {
    private:
        // Your private members go here
        //块头
        class BL_Head {
        public:
            //存储类型(0普通 1叶子)
            bool BL_type = false;
            off_t size = 0;
            off_t pos = 0;
            off_t parent = 0;
            off_t last = 0;
            off_t next = 0;
        };

        //索引数据
        struct Normal_Dt_Node {
            off_t son = 0;
            K k;
        };

        //B+树大数据块大小
        constexpr static off_t BL_SZ = 4096;
        //大数据块预留数据块大小
        constexpr static off_t IN_SZ = sizeof(BL_Head);
        //Key类型的大小
        constexpr static off_t K_SZ = sizeof(K);
        //Val类型的大小
        constexpr static off_t VAL_SZ = sizeof(Val);
        //大数据块能够存储孩子的个数(M)
        constexpr static off_t BL_K_N = (BL_SZ - IN_SZ) / sizeof(Normal_Dt_Node) - 1;
        //小数据块能够存放的记录的个数(L)
        constexpr static off_t BL_P_N = (BL_SZ - IN_SZ) / (K_SZ + VAL_SZ) - 1;

        //私有类
        //B+树文件头
        class  FL_Head {
        public:
            //存储BLOCK占用的空间
            off_t bl_cnt = 1;
            //存储根节点的位置
            off_t rt_p = 0;
            //存储数据块头
            off_t dt_bl_hd = 0;
            //存储数据块尾
            off_t dt_bl_rr = 0;
            //存储大小
            off_t sz = 0;
        };

        class Normal_Dt {
        public:
            Normal_Dt_Node val[BL_K_N];
        };

        //叶子数据
        class Lf_Dt {
        public:
            pair<K, Val> val[BL_PAIR_N];
        };

        //私有变量
        //文件头
        FL_Head tr_dt;

        //文件指针
        static FILE* fp;

        //私有函数
        //块内存读取
        template <class M_TP>
        static void m_rd(M_TP bf, off_t bf_sz, off_t pos) {
            fseek(fp, long(bf_sz * pos), SEEK_SET);
            fread(bf, bf_sz, 1, fp);
        }

        //块内存写入
        template <class M_TP>
        static void m_wr(M_TP bf, off_t bf_sz, off_t pos) {
            fseek(fp, long(bf_sz * pos), SEEK_SET);
            fwrite(bf, bf_sz, 1, fp);
            fflush(fp);
        }

        //写入B+树基本数据
        void wr_tr_dt() {
            fseek(fp, 0, SEEK_SET);
            char bf[BL_SZ] = { 0 };
            memcpy(bf, &tr_dt, sizeof(tr_dt));
            m_wr(bf, BL_SZ, 0);
        }

        //获取新内存
        off_t memory_allocation() {
            ++tr_dt.bl_cnt;
            wr_tr_dt();
            char bf[BL_SZ] = { 0 };
            m_wr(bf, BL_SZ, tr_dt.bl_cnt - 1);
            return tr_dt.bl_cnt - 1;
        }

        //创建新的索引结点
        off_t create_normal_node(off_t parent) {
            auto node_pos = memory_allocation();
            BL_Head temp;
            Normal_Dt normal_dt;
            temp.BL_type = false;
            temp.parent = parent;
            temp.pos = node_pos;
            temp.sz = 0;
            wr_block(&temp, &normal_dt, node_pos);
            return node_pos;
        }

        //创建新的叶子结点
        off_t create_lf_node(off_t parent, off_t last, off_t next) {
            auto node_pos = memory_allocation();
            BL_Head temp;
            Lf_Dt lf_dt;
            temp.BL_type = true;
            temp.parent = parent;
            temp.pos = node_pos;
            temp.last = last;
            temp.next = next;
            temp.sz = 0;
            wr_block(&temp, &lf_dt, node_pos);
            return node_pos;
        }

        //索引节点插入新索引
        void insert_new_index(BL_Head& parent_info, Normal_Dt& parent_dt,
                              off_t origin, off_t new_pos, const K& new_index) {
            ++parent_info.sz;
            auto p = parent_info.sz - 2;
            while (parent_dt.val[p].son != origin) {
                parent_dt.val[p + 1] = parent_dt.val[p];
                --p;
            }
            parent_dt.val[p + 1].k = parent_dt.val[p].k;
            parent_dt.val[p].k = new_index;
            parent_dt.val[p + 1].son = new_pos;
        }

        //读取结点信息
        template <class DT_TYPE>
        static void read_block(BL_Head* info, DT_TYPE* dt, off_t pos)
        {
            char bf[BL_SZ] = { 0 };
            m_rd(bf, BL_SZ, pos);
            memcpy(info, bf, sizeof(BL_Head));
            memcpy(dt, bf + IN_SZ, sizeof(DT_TYPE));
        }
        //写入节点信息
        template <class DT_TYPE>
        static void wr_block(BL_Head* info, DT_TYPE* dt, off_t pos) {
            char bf[BL_SZ] = { 0 };
            memcpy(bf, info, sizeof(BL_Head));
            memcpy(bf + IN_SZ, dt, sizeof(DT_TYPE));
            m_wr(bf, BL_SZ, pos);
        }

        //创建文件
        void check_file() {
            if (!fp) {
                //创建新的树
                fp = fopen(b+t ad, "wb+");
                wr_tr_dt();

                auto node_head = tr_dt.bl_cnt,
                        node_rear = tr_dt.bl_cnt + 1;

                tr_dt.dt_bl_hd = node_head;
                tr_dt.dt_bl_rr = node_rear;

                create_lf_node(0, 0, node_rear);
                create_lf_node(0, node_head, 0);

                return;
            }
            char bf[BL_SZ] = { 0 };
            m_rd(bf, BL_SZ, 0);
            memcpy(&tr_dt, bf, sizeof(tr_dt));
        }


        //分裂叶子结点
        K split_lf_node(off_t pos, BL_Head& origin_info, Lf_Dt& origin_dt) {
            //读入数据
            off_t parent_pos;
            BL_Head parent_info;
            Normal_Dt parent_dt;

            //判断是否为根结点
            if (pos == tr_dt.rt_p) {
                //创建根节点
                auto rt_p = create_normal_node(0);
                tr_dt.rt_p = rt_p;
                wr_tr_dt();
                read_block(&parent_info, &parent_dt, rt_p);
                origin_info.parent = rt_p;
                ++parent_info.sz;
                parent_dt.val[0].son = pos;
                parent_pos = rt_p;
            }
            else {
                read_block(&parent_info, &parent_dt, origin_info.parent);
                parent_pos = parent_info.pos;
            }
            if (split_parent(origin_info)) {
                parent_pos = origin_info.parent;
                read_block(&parent_info, &parent_dt, parent_pos);
            }
            //创建一个新的子结点
            auto new_pos = create_lf_node(parent_pos,pos,origin_info.next);

            //修改后继结点的前驱
            auto tmp_pos = origin_info.next;
            BL_Head tmp_info;
            Lf_Dt tmp_dt;
            read_block(&tmp_info, &tmp_dt, tmp_pos);
            tmp_info.last = new_pos;
            wr_block(&tmp_info, &tmp_dt, tmp_pos);
            origin_info.next = new_pos;

            BL_Head new_info;
            Lf_Dt new_dt;
            read_block(&new_info, &new_dt, new_pos);

            //移动数据的位置
            off_t mid_pos = origin_info.sz >> 1;
            for (off_t p = mid_pos, i = 0; p < origin_info.sz; ++p, ++i) {
                new_dt.val[i].first = origin_dt.val[p].first;
                new_dt.val[i].second = origin_dt.val[p].second;
                ++new_info.sz;
            }
            origin_info.sz = mid_pos;
            insert_new_index(parent_info, parent_dt, pos, new_pos, origin_dt.val[mid_pos].first);

            //写入
            wr_block(&origin_info, &origin_dt, pos);
            wr_block(&new_info, &new_dt, new_pos);
            wr_block(&parent_info, &parent_dt, parent_pos);

            return new_dt.val[0].first;
        }

        //分裂父亲（返回新的父亲）
        bool split_parent(BL_Head& son_info) {
            //读入数据
            off_t parent_pos, origin_pos = son_info.parent;
            BL_Head parent_info, origin_info;
            Normal_Dt parent_dt, origin_dt;
            read_block(&origin_info, &origin_dt, origin_pos);
            if (origin_info.sz < BL_K_N)
                return false;

            //判断是否为根结点
            if (origin_pos == tr_dt.rt_p) {
                //创建根节点
                auto rt_p = create_normal_node(0);
                tr_dt.rt_p = rt_p;
                wr_tr_dt();
                read_block(&parent_info, &parent_dt, rt_p);
                origin_info.parent = rt_p;
                ++parent_info.sz;
                parent_dt.val[0].son = origin_pos;
                parent_pos = rt_p;
            }
            else {
                read_block(&parent_info, &parent_dt, origin_info.parent);
                parent_pos = parent_info.pos;
            }
            if (split_parent(origin_info)) {
                parent_pos = origin_info.parent;
                read_block(&parent_info, &parent_dt, parent_pos);
            }
            //创建一个新的子结点
            auto new_pos = create_normal_node(parent_pos);
            BL_Head new_info;
            Normal_Dt new_dt;
            read_block(&new_info, &new_dt, new_pos);

            //移动数据的位置
            off_t mid_pos = origin_info.sz >> 1;
            for (off_t p = mid_pos + 1, i = 0; p < origin_info.sz; ++p,++i) {
                if (origin_dt.val[p].son == son_info.pos) {
                    son_info.parent = new_pos;
                }
                std::swap(new_dt.val[i], origin_dt.val[p]);
                ++new_info.sz;
            }
            origin_info.sz = mid_pos + 1;
            insert_new_index(parent_info, parent_dt, origin_pos, new_pos, origin_dt.val[mid_pos].k);

            //写入
            wr_block(&origin_info, &origin_dt, origin_pos);
            wr_block(&new_info, &new_dt, new_pos);
            wr_block(&parent_info, &parent_dt, parent_pos);
            return true;
        }

        //合并索引
        void merge_normal(BL_Head& l_info, Normal_Dt& l_dt, BL_Head& r_info, Normal_Dt& r_dt) {
            for (off_t p = l_info.sz, i = 0; i < r_info.sz; ++p, ++i) {
                l_dt.val[p] = r_dt.val[i];
            }
            l_dt.val[l_info.sz - 1].k = adjust_normal(r_info.parent, r_info.pos);
            l_info.sz += r_info.sz;
            wr_block(&l_info, &l_dt, l_info.pos);
        }
        //修改LCA的索引(平衡叶子时)
        void change_index(off_t parent, off_t son, const K& new_k) {
            //读取数据
            BL_Head parent_info;
            Normal_Dt parent_dt;
            read_block(&parent_info, &parent_dt, parent);
            if (parent_dt.val[parent_info.sz - 1].son == son) {
                change_index(parent_info.parent, parent, new_k);
                return;
            }
            off_t cur_pos = parent_info.sz - 2;
            while (true) {
                if (parent_dt.val[cur_pos].son == son) {
                    parent_dt.val[cur_pos].k = new_k;
                    break;
                }
                --cur_pos;
            }
            wr_block(&parent_info, &parent_dt, parent);
        }

        //平衡索引
        void balance_normal(BL_Head& info, Normal_Dt& normal_dt) {
            if (info.sz >= BL_K_N / 2) {
                wr_block(&info, &normal_dt, info.pos);
                return;
            }
            //判断是否是根
            if (info.pos == tr_dt.rt_p && info.sz <= 1) {
                tr_dt.rt_p = normal_dt.val[0].son;
                wr_tr_dt();
                return;
            }
            else if (info.pos == tr_dt.rt_p) {
                wr_block(&info, &normal_dt, info.pos);
                return;
            }
            //获取兄弟
            BL_Head parent_info, brother_info;
            Normal_Dt parent_dt, brother_dt;
            read_block(&parent_info, &parent_dt, info.parent);
            off_t value_pos;
            for (value_pos = 0; parent_dt.val[value_pos].son != info.pos; ++value_pos);
            if (value_pos > 0) {
                read_block(&brother_info, &brother_dt, parent_dt.val[value_pos - 1].son);
                brother_info.parent = info.parent;
                if (brother_info.sz > BL_K_N / 2) {
                    off_t p = info.sz;
                    while (p > 0) {
                        normal_dt.val[p] = normal_dt.val[p - 1];
                        --p;
                    }
                    normal_dt.val[0].son = brother_dt.val[brother_info.sz - 1].son;
                    normal_dt.val[0].k = parent_dt.val[value_pos - 1].k;
                    parent_dt.val[value_pos - 1].k = brother_dt.val[brother_info.sz - 2].k;
                    --brother_info.sz;
                    ++info.sz;
                    wr_block(&brother_info, &brother_dt, brother_info.pos);
                    wr_block(&info, &normal_dt, info.pos);
                    wr_block(&parent_info, &parent_dt, parent_info.pos);
                    return;
                }
                else {
                    merge_normal(brother_info, brother_dt, info, normal_dt);
                    return;
                }
            }
            if (value_pos < parent_info.sz - 1) {
                read_block(&brother_info, &brother_dt, parent_dt.val[value_pos + 1].son);
                brother_info.parent = info.parent;
                if (brother_info.sz > BL_K_N / 2) {
                    normal_dt.val[info.sz].son = brother_dt.val[0].son;
                    normal_dt.val[info.sz - 1].k = parent_dt.val[value_pos].k;
                    parent_dt.val[value_pos].k = brother_dt.val[0].k;
                    for (off_t p = 1; p < brother_info.sz; ++p)
                    {
                        brother_dt.val[p - 1] = brother_dt.val[p];
                    }
                    --brother_info.sz;
                    ++info.sz;
                    wr_block(&brother_info, &brother_dt, brother_info.pos);
                    wr_block(&info, &normal_dt, info.pos);
                    wr_block(&parent_info, &parent_dt, parent_info.pos);
                    return;
                }
                else {
                    merge_normal(info, normal_dt, brother_info, brother_dt);
                    return;
                }
            }
        }

        //调整索引(返回关键字)
        K adjust_normal(off_t pos, off_t removed_son) {
            BL_Head info;
            Normal_Dt normal_dt;
            read_block(&info, &normal_dt, pos);
            off_t cur_pos;
            for (cur_pos = 0; normal_dt.val[cur_pos].son != removed_son; ++cur_pos);
            K ans = normal_dt.val[cur_pos - 1].k;
            normal_dt.val[cur_pos - 1].k = normal_dt.val[cur_pos].k;
            while(cur_pos < info.sz - 1)
            {
                normal_dt.val[cur_pos] = normal_dt.val[cur_pos + 1];
                ++cur_pos;
            }
            --info.sz;
            balance_normal(info, normal_dt);
            return ans;
        }

        //合并叶子
        void merge_lf(BL_Head& l_info, Lf_Dt& l_dt, BL_Head& r_info, Lf_Dt& r_dt) {
            for (off_t p = l_info.sz, i = 0; i < r_info.sz; ++p, ++i) {
                l_dt.val[p].first = r_dt.val[i].first;
                l_dt.val[p].second = r_dt.val[i].second;
            }
            l_info.sz += r_info.sz;
            adjust_normal(r_info.parent, r_info.pos);
            //修改链表
            l_info.next = r_info.next;
            BL_Head temp_info;
            Lf_Dt temp_dt;
            read_block(&temp_info, &temp_dt, r_info.next);
            temp_info.last = l_info.pos;
            wr_block(&temp_info, &temp_dt, temp_info.pos);
            wr_block(&l_info, &l_dt, l_info.pos);
        }

        //平衡叶子
        void balance_lf(BL_Head& lf_info, Lf_Dt& lf_dt) {
            if (lf_info.sz >= BL_PAIR_N / 2) {
                wr_block(&lf_info, &lf_dt, lf_info.pos);
                return;
            }
            else if (lf_info.pos == tr_dt.rt_p) {
                if (lf_info.sz == 0) {
                    BL_Head temp_info;
                    Lf_Dt temp_dt;
                    read_block(&temp_info, &temp_dt, tr_dt.dt_bl_hd);
                    temp_info.next = tr_dt.dt_bl_rr;
                    wr_block(&temp_info, &temp_dt, tr_dt.dt_bl_hd);
                    read_block(&temp_info, &temp_dt, tr_dt.dt_bl_rr);
                    temp_info.last = tr_dt.dt_bl_hd;
                    wr_block(&temp_info, &temp_dt, tr_dt.dt_bl_rr);
                    return;
                }
                wr_block(&lf_info, &lf_dt, lf_info.pos);
                return;
            }
            //查找兄弟结点
            BL_Head brother_info, parent_info;
            Lf_Dt brother_dt;
            Normal_Dt parent_dt;

            read_block(&parent_info, &parent_dt, lf_info.parent);
            off_t node_pos = 0;
            while (node_pos < parent_info.sz)
            {
                if (parent_dt.val[node_pos].son == lf_info.pos)
                    break;
                ++node_pos;
            }

            //左兄弟
            if (node_pos > 0) {
                read_block(&brother_info, &brother_dt, lf_info.last);
                brother_info.parent = lf_info.parent;
                if (brother_info.sz > BL_PAIR_N / 2) {
                    for (off_t p = lf_info.sz; p > 0; --p) {
                        lf_dt.val[p].first = lf_dt.val[p - 1].first;
                        lf_dt.val[p].second = lf_dt.val[p - 1].second;
                    }
                    lf_dt.val[0].first = brother_dt.val[brother_info.sz - 1].first;
                    lf_dt.val[0].second = brother_dt.val[brother_info.sz - 1].second;
                    --brother_info.sz;
                    ++lf_info.sz;
                    change_index(brother_info.parent, brother_info.pos, lf_dt.val[0].first);
                    wr_block(&brother_info, &brother_dt, brother_info.pos);
                    wr_block(&lf_info, &lf_dt, lf_info.pos);
                    return;
                }
                else {
                    merge_lf(brother_info, brother_dt, lf_info, lf_dt);
                    //wr_block(&brother_info, &brother_dt, brother_info._pos);
                    return;
                }
            }
            //右兄弟
            if (node_pos < parent_info.sz - 1) {
                read_block(&brother_info, &brother_dt, lf_info.next);
                brother_info.parent = lf_info.parent;
                if (brother_info.sz > BL_PAIR_N / 2) {
                    lf_dt.val[lf_info.sz].first = brother_dt.val[0].first;
                    lf_dt.val[lf_info.sz].second = brother_dt.val[0].second;
                    for (off_t p = 1; p < brother_info.sz; ++p) {
                        brother_dt.val[p - 1].first = brother_dt.val[p].first;
                        brother_dt.val[p - 1].second = brother_dt.val[p].second;
                    }
                    ++lf_info.sz;
                    --brother_info.sz;
                    change_index(lf_info.parent, lf_info.pos, brother_dt.val[0].first);
                    wr_block(&lf_info, &lf_dt, lf_info.pos);
                    wr_block(&brother_info, &brother_dt, brother_info.pos);
                    return;
                }
                else {
                    merge_lf(lf_info, lf_dt, brother_info, brother_dt);
                    return;
                }
            }
        }

    public:
        typedef pair<const K, Val> value_type;

        class const_iterator;
        class iterator {
            friend class sjtu::BTree<K, Val, Cmp>::const_iterator;
            friend iterator sjtu::BTree<K, Val, Cmp>::begin();
            friend iterator sjtu::BTree<K, Val, Cmp>::end();
            friend iterator sjtu::BTree<K, Val, Cmp>::find(const K&);
            friend pair<iterator, OperationResult> sjtu::BTree<K, Val, Cmp>::insert(const K&, const Value&);
        private:
            // Your private members go here
            //指向当前bpt
            BTree* cur_bptree = nullptr;
            //存储当前块的基本信息
            BL_Head block_info;
            //存储当前指向的元素位置
            off_t cur_pos = 0;

        public:
            bool modify(const Val& value) {
                BL_Head info;
                Lf_Dt lf_dt;
                read_block(&info, &lf_dt, block_info.pos);
                lf_dt.val[cur_pos].second = value;
                wr_block(&info, &lf_dt, block_info.pos);
                return true;
            }
            iterator() {
                // TODO Default Constructor
            }
            iterator(const iterator& other) {
                // TODO Copy Constructor
                cur_bptree = other.cur_bptree;
                block_info = other.block_info;
                cur_pos = other.cur_pos;
            }
            // Return a new iterator which points to the n-next elements
            iterator operator++(int) {
                // Todo iterator++
                auto tmp = *this;
                ++cur_pos;
                if (cur_pos >= block_info.sz) {
                    char bf[BL_SZ] = { 0 };
                    m_rd(bf, BL_SZ, block_info.next);
                    memcpy(&block_info, bf, sizeof(block_info));
                    cur_pos = 0;
                }
                return tmp;
            }
            iterator& operator++() {
                // Todo ++iterator
                ++cur_pos;
                if (cur_pos >= block_info.sz) {
                    char bf[BL_SZ] = { 0 };
                    m_rd(bf, BL_SZ, block_info.next);
                    memcpy(&block_info, bf, sizeof(block_info));
                    cur_pos = 0;
                }
                return *this;
            }
            iterator operator--(int) {
                // Todo iterator--
                auto temp = *this;
                if (cur_pos == 0) {
                    char bf[BL_SZ] = { 0 };
                    m_rd(bf, BL_SZ, block_info.last);
                    memcpy(&block_info, bf, sizeof(block_info));
                    cur_pos = block_info.sz - 1;
                }
                else
                    --cur_pos;
                return temp;
            }
            iterator& operator--() {
                // Todo --iterator
                if (cur_pos == 0) {
                    char bf[BL_SZ] = { 0 };
                    m_rd(bf, BL_SZ, block_info.last);
                    memcpy(&block_info, bf, sizeof(block_info));
                    cur_pos = block_info.sz - 1;
                }
                else
                    --cur_pos;

                return *this;
            }
            // Overloaded of operator '==' and '!='
            // Check whether the iterators are same
            value_type operator*() const {
                // Todo operator*, return the <K,V> of iterator
                if (cur_pos >= block_info.sz)
                    throw invalid_iterator();
                char bf[BL_SZ] = { 0 };
                m_rd(bf, BL_SZ, block_info.pos);
                Lf_Dt lf_dt;
                memcpy(&lf_dt, bf + IN_SZ, sizeof(lf_dt));
                value_type result(lf_dt.val[cur_pos].first,lf_dt.val[cur_pos].second);
                return result;
            }
            bool operator==(const iterator& rhs) const {
                // Todo operator ==
                return cur_bptree == rhs.cur_bptree
                       && block_info.pos == rhs.block_info.pos
                       && cur_pos == rhs.cur_pos;
            }
            bool operator==(const const_iterator& rhs) const {
                // Todo operator ==
                return block_info.pos == rhs.block_info.pos
                       && cur_pos == rhs.cur_pos;
            }
            bool operator!=(const iterator& rhs) const {
                // Todo operator !=
                return cur_bptree != rhs.cur_bptree
                       || block_info.pos != rhs.block_info.pos
                       || cur_pos != rhs.cur_pos;
            }
            bool operator!=(const const_iterator& rhs) const {
                // Todo operator !=
                return block_info.pos != rhs.block_info.pos
                       || cur_pos != rhs.cur_pos;
            }
        };
        class const_iterator {
            // it should has similar member method as iterator.
            //  and it should be able to construct from an iterator.
            friend class sjtu::BTree<K, Val, Cmp>::iterator;
            friend const_iterator sjtu::BTree<K, Val, Cmp>::cbegin() const;
            friend const_iterator sjtu::BTree<K, Val, Cmp>::cend() const;
            friend const_iterator sjtu::BTree<K, Val, Cmp>::find(const K&) const;
        private:
            // Your private members go here
            //存储当前块的基本信息
            BL_Head block_info;
            //存储当前指向的元素位置
            off_t cur_pos = 0;
        public:
            const_iterator() {
                // TODO
            }
            const_iterator(const const_iterator& other) {
                // TODO
                block_info = other.block_info;
                cur_pos = other.cur_pos;
            }
            const_iterator(const iterator& other) {
                // TODO
                block_info = other.block_info;
                cur_pos = other.cur_pos;
            }
            // And other methods in iterator, please fill by yourself.
            // Return a new iterator which points to the n-next elements
            const_iterator operator++(int) {
                // Todo iterator++
                auto tmp = *this;
                ++cur_pos;
                if (cur_pos >= block_info.sz) {
                    char bf[BL_SZ] = { 0 };
                    m_rd(bf, BL_SZ, block_info.next);
                    memcpy(&block_info, bf, sizeof(block_info));
                    cur_pos = 0;
                }
                return tmp;
            }
            const_iterator& operator++() {
                // Todo ++iterator
                ++cur_pos;
                if (cur_pos >= block_info.sz) {
                    char bf[BL_SZ] = { 0 };
                    m_rd(bf, BL_SZ, block_info.next);
                    memcpy(&block_info, bf, sizeof(block_info));
                    cur_pos = 0;
                }
                return *this;
            }
            const_iterator operator--(int) {
                // Todo iterator--
                auto tmp = *this;
                if (cur_pos == 0) {
                    char bf[BL_SZ] = { 0 };
                    m_rd(bf, BL_SZ, block_info.last);
                    memcpy(&block_info, bf, sizeof(block_info));
                    cur_pos = block_info.sz - 1;
                }
                else
                    --cur_pos;
                return tmp;
            }
            const_iterator& operator--() {
                // Todo --iterator
                if (cur_pos == 0) {
                    char bf[BL_SZ] = { 0 };
                    m_rd(bf, BL_SZ, block_info.last);
                    memcpy(&block_info, bf, sizeof(block_info));
                    cur_pos = block_info.sz - 1;
                }
                else
                    --cur_pos;

                return *this;
            }
            // Overloaded of operator '==' and '!='
            // Check whether the iterators are same
            value_type operator*() const {
                // Todo operator*, return the <K,V> of iterator
                if (cur_pos >= block_info.sz)
                    throw invalid_iterator();
                char bf[BL_SZ] = { 0 };
                m_rd(bf, BL_SZ, block_info.pos);
                Lf_Dt lf_dt;
                memcpy(&lf_dt, bf + IN_SZ, sizeof(lf_dt));
                value_type result(lf_dt.val[cur_pos].first, lf_dt.val[cur_pos].second);
                return result;
            }
            bool operator==(const iterator& rhs) const {
                // Todo operator ==
                return block_info.pos == rhs.block_info.pos
                       && cur_pos == rhs.cur_pos;
            }
            bool operator==(const const_iterator& rhs) const {
                // Todo operator ==
                return block_info.pos == rhs.block_info.pos
                       && cur_pos == rhs.cur_pos;
            }
            bool operator!=(const iterator& rhs) const {
                // Todo operator !=
                return block_info.pos != rhs.block_info.pos
                       || cur_pos != rhs.cur_pos;
            }
            bool operator!=(const const_iterator& rhs) const {
                // Todo operator !=
                return block_info.pos != rhs.block_info.pos
                       || cur_pos != rhs.cur_pos;
            }
        };
        // Default Constructor and Copy Constructor
        BTree() {
            // Todo Default
            fp = fopen(b+t ad, "rb+");
            if (!fp) {
                //创建新的树
                fp = fopen(b+t ad, "wb+");
                wr_tr_dt();

                auto node_head = tr_dt.bl_cnt,
                        node_rear = tr_dt.bl_cnt + 1;

                tr_dt.dt_bl_hd = node_head;
                tr_dt.dt_bl_rr = node_rear;

                create_lf_node(0, 0, node_rear);
                create_lf_node(0, node_head, 0);

                return;
            }
            char bf[BL_SZ] = { 0 };
            m_rd(bf, BL_SZ, 0);
            memcpy(&tr_dt, bf, sizeof(tr_dt));
        }
        BTree(const BTree& other) {
            // Todo Copy
            fp = fopen(b+t ad, "rb+");
            tr_dt.bl_cnt = other.tr_dt.bl_cnt;
            tr_dt.dt_bl_hd = other.tr_dt.dt_bl_hd;
            tr_dt.dt_bl_rr = other.tr_dt.dt_bl_rr;
            tr_dt.rt_p = other.tr_dt.rt_p;
            tr_dt.sz = other.tr_dt.sz;
        }
        BTree& operator=(const BTree& other) {
            // Todo Assignment
            fp = fopen(b+t ad, "rb+");
            tr_dt.bl_cnt = other.tr_dt.bl_cnt;
            tr_dt.dt_bl_hd = other.tr_dt.dt_bl_hd;
            tr_dt.dt_bl_rr = other.tr_dt.dt_bl_rr;
            tr_dt.rt_p = other.tr_dt.rt_p;
            tr_dt.sz = other.tr_dt.sz;
            return *this;
        }
        ~BTree() {
            // Todo Destructor
            fclose(fp);
        }
        // Insert: Insert certain K-Val into the database
        // Return a pair, the first of the pair is the iterator point to the new
        // element, the second of the pair is Success if it is successfully inserted
        pair<iterator, OperationResult> insert(const K& k, const Val& value) {
            // TODO insert function
            check_file();
            if (empty()) {
                auto rt_p = create_lf_node(0, tr_dt.dt_bl_hd, tr_dt.dt_bl_rr);

                BL_Head temp_info;
                Lf_Dt temp_dt;
                read_block(&temp_info, &temp_dt, tr_dt.dt_bl_hd);
                temp_info.next = rt_p;
                wr_block(&temp_info, &temp_dt, tr_dt.dt_bl_hd);

                read_block(&temp_info, &temp_dt, tr_dt.dt_bl_rr);
                temp_info.last = rt_p;
                wr_block(&temp_info, &temp_dt, tr_dt.dt_bl_rr);

                read_block(&temp_info, &temp_dt, rt_p);
                ++temp_info.sz;
                temp_dt.val[0].first = k;
                temp_dt.val[0].second = value;
                wr_block(&temp_info, &temp_dt, rt_p);

                ++tr_dt.sz;
                tr_dt.rt_p = rt_p;
                wr_tr_dt();

                pair<iterator, OperationResult> result(begin(), Success);
                return result;
            }

            //查找正确的节点位置
            char bf[BL_SZ] = { 0 };
            off_t cur_pos = tr_dt.rt_p, cur_parent = 0;
            while (true) {
                m_rd(bf, BL_SZ, cur_pos);
                BL_Head temp;
                memcpy(&temp, bf, sizeof(temp));
                //判断父亲是否更新
                if (cur_parent != temp.parent) {
                    temp.parent = cur_parent;
                    memcpy(bf, &temp, sizeof(temp));
                    m_wr(bf, BL_SZ, cur_pos);
                }
                if (temp.BL_type) {
                    break;
                }
                Normal_Dt normal_dt;
                memcpy(&normal_dt, bf + IN_SZ, sizeof(normal_dt));
                off_t son_pos = temp.sz - 1;
                while (son_pos > 0) {
                    if (normal_dt.val[son_pos - 1].k <= k) break;
                    --son_pos;
                }
                cur_parent = cur_pos;
                cur_pos = normal_dt.val[son_pos].son;
                cur_pos = cur_pos;
            }

            BL_Head info;
            memcpy(&info, bf, sizeof(info));
            Lf_Dt lf_dt;
            memcpy(&lf_dt, bf + IN_SZ, sizeof(lf_dt));
            for (off_t value_pos = 0;; ++value_pos) {
                if (value_pos < info.sz && (!(lf_dt.val[value_pos].first < k || lf_dt.val[value_pos].first > k))) {
                    return pair<iterator, OperationResult>(end(), Fail);
                }
                if (value_pos >= info.sz || lf_dt.val[value_pos].first > k) {
                    //在此结点之前插入
                    if (info.sz >= BL_PAIR_N) {
                        auto cur_k = split_lf_node(cur_pos, info, lf_dt);
                        if (k > cur_k) {
                            cur_pos = info.next;
                            value_pos -= info.sz;
                            read_block(&info, &lf_dt, cur_pos);
                        }
                    }

                    for (off_t p = info.sz - 1; p >= value_pos; --p)
                    {
                        lf_dt.val[p + 1].first = lf_dt.val[p].first;
                        lf_dt.val[p + 1].second = lf_dt.val[p].second;
                        if (p == value_pos)
                            break;
                    }
                    lf_dt.val[value_pos].first = k;
                    lf_dt.val[value_pos].second = value;
                    ++info.sz;
                    wr_block(&info, &lf_dt, cur_pos);
                    iterator ans;
                    ans.block_info = info;
                    ans.cur_bptree = this;
                    ans.cur_pos = value_pos;
                    //修改树的基本参数
                    ++tr_dt.sz;
                    wr_tr_dt();
                    pair<iterator, OperationResult> re(ans, Success);
                    return re;
                }
            }
            return pair<iterator, OperationResult>(end(), Fail);
        }
        // Erase: Erase the K-Val
        // Return Success if it is successfully erased
        // Return Fail if the k doesn't exist in the database
        OperationResult erase(const K& k) {
            return Fail;
        }
        iterator begin() {
            check_file();
            iterator result;
            char bf[BL_SZ] = { 0 };
            m_rd(bf, BL_SZ, tr_dt.dt_bl_hd);
            BL_Head block_head;
            memcpy(&block_head, bf, sizeof(block_head));
            result.block_info = block_head;
            result.cur_bptree = this;
            result.cur_pos = 0;
            ++result;
            return result;
        }
        const_iterator cbegin() const {
            const_iterator result;
            char bf[BL_SZ] = { 0 };
            m_rd(bf, BL_SZ, tr_dt.dt_bl_hd);
            BL_Head block_head;
            memcpy(&block_head, bf, sizeof(block_head));
            result.block_info = block_head;
            result.cur_pos = 0;
            ++result;
            return result;
        }
        // Return a iterator to the end(the next element after the last)
        iterator end() {
            check_file();
            iterator result;
            char bf[BL_SZ] = { 0 };
            m_rd(bf, BL_SZ, tr_dt.dt_bl_rr);
            BL_Head block_head;
            memcpy(&block_head, bf, sizeof(block_head));
            result.block_info = block_head;
            result.cur_bptree = this;
            result.cur_pos = 0;
            return result;
        }
        const_iterator cend() const {
            const_iterator result;
            char bf[BL_SZ] = { 0 };
            m_rd(bf, BL_SZ, tr_dt.dt_bl_rr);
            BL_Head block_head;
            memcpy(&block_head, bf, sizeof(block_head));
            result.block_info = block_head;
            result.cur_pos = 0;
            return result;
        }
        // Check whether this BTree is empty
        bool empty() const {
            if (!fp)
                return true;
            return tr_dt.sz == 0;
        }
        // Return the number of <K,V> pairs
        off_t sz() const {
            if (!fp)
                return 0;
            return tr_dt.sz;
        }
        // Clear the BTree
        void clear() {
            if (!fp)
                return;
            remove(b+t ad);
            FL_Head new_fl_hd;
            tr_dt = new_fl_hd;
            fp = nullptr;
        }
        // Return the value refer to the K(k)
        Val at(const K& k) {
            if (empty()) {
                throw container_is_empty();
            }
            //查找正确的节点位置
            char bf[BL_SZ] = { 0 };
            off_t cur_pos = tr_dt.rt_p, cur_parent = 0;
            while (true) {
                m_rd(bf, BL_SZ, cur_pos);
                BL_Head temp;
                memcpy(&temp, bf, sizeof(temp));
                //判断父亲是否更新
                if (cur_parent != temp.parent) {
                    temp.parent = cur_parent;
                    memcpy(bf, &temp, sizeof(temp));
                    m_wr(bf, BL_SZ, cur_pos);
                }
                if (temp.BL_type) break;

                Normal_Dt normal_dt;
                memcpy(&normal_dt, bf + IN_SZ, sizeof(normal_dt));
                off_t son_pos = temp.sz - 1;
                for (; son_pos > 0; --son_pos) {
                    if (normal_dt.val[son_pos - 1].k <= k) {
                        break;
                    }
                }
                cur_pos = normal_dt.val[son_pos].son;
            }
            BL_Head info;
            memcpy(&info, bf, sizeof(info));
            Lf_Dt lf_dt;
            memcpy(&lf_dt, bf + IN_SZ, sizeof(lf_dt));
            for (off_t value_pos = 0;; ++value_pos) {
                if (value_pos < info.sz && (!(lf_dt.val[value_pos].first<k || lf_dt.val[value_pos].first>k))) {
                    return lf_dt.val[value_pos].second;
                }
                if (value_pos >= info.sz || lf_dt.val[value_pos].first > k) {
                    throw index_out_of_bound();
                }
            }
        }

        /**
         * Returns the number of elements with k
         *   that compares equivalent to the specified argument,
         * The default method of check the equivalence is !(a < b || b > a)
         */
        off_t count(const K& k) const {
            return find(k) == cend() ? 0 : 1;
        }

        /**
         * Finds an element with k equivalent to k.
         * k value of the element to search for.
         * Iterator to an element with k equivalent to k.
         *   If no such element is found, past-the-end (see end()) iterator is
         * returned.
         */
        iterator find(const K& k) {
            if (empty()) {
                return end();
            }
            //查找正确的节点位置
            char bf[BL_SZ] = { 0 };
            off_t cur_pos = tr_dt.rt_p, cur_parent = 0;
            while (true) {
                m_rd(bf, BL_SZ, cur_pos);
                BL_Head temp;
                memcpy(&temp, bf, sizeof(temp));
                //判断父亲是否更新
                if (cur_parent != temp.parent) {
                    temp.parent = cur_parent;
                    memcpy(bf, &temp, sizeof(temp));
                    mem_wr(bf, BL_SZ, cur_pos);
                }
                if (temp.BL_type) {
                    break;
                }
                Normal_Dt normal_dt;
                memcpy(&normal_dt, bf + IN_SZ, sizeof(normal_dt));
                off_t son_pos = temp.sz - 1;
                for (; son_pos > 0; --son_pos) {
                    if (!(normal_dt.val[son_pos - 1].k > k)) {
                        break;
                    }
                }
                cur_pos = normal_dt.val[son_pos].son;
            }
            BL_Head info;
            memcpy(&info, bf, sizeof(info));
            sizeof(Normal_Dt);
            Lf_Dt lf_dt;
            memcpy(&lf_dt, bf + IN_SZ, sizeof(lf_dt));
            for (off_t value_pos = 0;; ++value_pos) {
                if (value_pos < info.sz && (!(lf_dt.val[value_pos].first<k || lf_dt.val[value_pos].first>k))) {
                    iterator result;
                    result.cur_bptree = this;
                    result.block_info = info;
                    result.cur_pos = value_pos;
                    return result;
                }
                if (value_pos >= info.sz || lf_dt.val[value_pos].first > k) {
                    return end();
                }
            }
            return end();
        }
        const_iterator find(const K& k) const {
            if (empty()) {
                return cend();
            }
            //查找正确的节点位置
            char bf[BL_SZ] = { 0 };
            off_t cur_pos = tr_dt.rt_p, cur_parent = 0;
            while (true) {
                m_rd(bf, BL_SZ, cur_pos);
                BL_Head temp;
                memcpy(&temp, bf, sizeof(temp));
                //判断父亲是否更新
                if (cur_parent != temp.parent) {
                    temp.parent = cur_parent;
                    memcpy(bf, &temp, sizeof(temp));
                    mem_wr(bf, BL_SZ, cur_pos);
                }
                if (temp.BL_type) {
                    break;
                }
                Normal_Dt normal_dt;
                memcpy(&normal_dt, bf + IN_SZ, sizeof(normal_dt));
                off_t son_pos = temp.sz - 1;
                for (; son_pos > 0; --son_pos) {
                    if (!(normal_dt.val[son_pos - 1].k > k)) {
                        break;
                    }
                }
                cur_pos = normal_dt.val[son_pos].son;
            }
            BL_Head info;
            memcpy(&info, bf, sizeof(info));
            Lf_Dt lf_dt;
            memcpy(&lf_dt, bf + IN_SZ, sizeof(lf_dt));
            for (off_t value_pos = 0;; ++value_pos) {
                if (value_pos < info.sz && (!(lf_dt.val[value_pos].first<k || lf_dt.val[value_pos].first>k))) {
                    const_iterator result;
                    result.block_info = info;
                    result.cur_pos = value_pos;
                    return result;
                }
                if (value_pos >= info.sz || lf_dt.val[value_pos].first > k) {
                    return cend();
                }
            }
            return cend();
        }
    };
    template <typename K, typename Val, typename Cmp> FILE* BTree<K, Val, Cmp>::fp = nullptr;
}  // namespace sjtu
