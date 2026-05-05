#include <vector>
#include <iostream>
#include <algorithm>
using namespace std;

class node_bstar{
    public:
        node_bstar* parent;
        vector<node_bstar*> children; // 자식 노드 가리키는 포인터 벡터
        vector<int> rids; // rid
        vector<int> keys; // key
        // key 랑 rid 랑 1:1 로 같은거임
        bool is_leaf; // 끝인지 아닌지

        node_bstar(bool is_leaf){ this->is_leaf = is_leaf; parent = nullptr; }
};

class bstar_tree{
    private:
        node_bstar* root;
        int split_counter = 0; // split 횟수 세는 변수
        int order;
        int maxkeys;
        int minkeys;
    public:
        bstar_tree(int order){ this->order = order; root = nullptr; maxkeys = order - 1; minkeys = (2 * order + 2) / 3 - 1;}
        int get_split_count() { return split_counter; }
        node_bstar* get_root() { return root; }
        void insert(int key, int rid){
            if (root == nullptr){ // 트리가 없으면 트리 만들어야함}
                root = new node_bstar(true);
                root->keys.push_back(key);
                root->rids.push_back(rid);
                return;
            }
            // 트리는 있고 이제 진짜 삽입하는경우
            node_bstar* cur = root;
            while(!cur->is_leaf){ // 리프 노드 나올 때 까지
                   //중복된 키 없다고 가정하니까 그냥 자식 노드위치로 가면된다.
                   int idx = lower_bound(cur->keys.begin(), cur->keys.end(), key) - cur->keys.begin();
                   cur = cur->children[idx];
            }
            // 넣어야하는 리프노드 도착했음
            auto it = lower_bound(cur->keys.begin(), cur->keys.end(), key);
            int idx = it - cur->keys.begin();

            cur->keys.insert(it, key);
            cur->rids.insert(cur->rids.begin() + idx, rid); // 둘다 삽입함.

            // split 하고 부모노드 올리는거 반복문으로
            // cur 은 split 해야하는거 확인하는 노드
            // split 해야하는 경우인지 확인하고 아니면 끝나면 됨. 했으면 부모노드로 계속 올라가면서 반복확인.
            // 만약 부모도드가 없으면, split한거 올릴 새로운 루트노드 만들어서 끝내면 됨.
            // left mid right 으로 쪼개고 ( left 는 재사용, right 는 새로만들기)
            while( cur->keys.size() > maxkeys){// 스플릿 해야하는 경우
                // 왼쪽형제 여유 있을때
                if (cur->parent != nullptr){
                    node_bstar* parent = cur->parent;
                    auto it = find(parent->children.begin(), parent->children.end(), cur);
                    int idx = it - parent->children.begin();
                    if (idx > 0 && parent->children[idx - 1]->keys.size() < maxkeys){ // 왼쪽 형제 노드가 여유있으면
                        node_bstar* left_sibling = parent->children[idx - 1];
                        // cur 노드에서 가장 작은거 부모노드로 올리고 왼쪽 형제노드에 부모노드에서 idx - 1 키랑 rid 내려주고 cur 노드에서 가장 작은거 지워주기
                        left_sibling->keys.push_back(parent->keys[idx - 1]);
                        left_sibling->rids.push_back(parent->rids[idx - 1]);
                        parent->keys[idx - 1] = cur->keys.front();
                        parent->rids[idx - 1] = cur->rids.front();
                        if (!cur->is_leaf){ // 자식노드도 돌려줘야하는데 자식노드가 있으면
                            left_sibling->children.push_back(cur->children.front());
                            cur->children.erase(cur->children.begin());
                            left_sibling->children.back()->parent = left_sibling; // 자식노드도 부모노드 바꿔주기
                        }
                        cur->keys.erase(cur->keys.begin());
                        cur->rids.erase(cur->rids.begin());
                        break; // split 끝났으니까 반복문 탈출
                    }
                }

                // 오른쪽 형제 여유 있을때
                if (cur->parent != nullptr){
                    node_bstar* parent = cur->parent;
                    auto it = find(parent->children.begin(), parent->children.end(), cur);
                    int idx = it - parent->children.begin();
                    if (idx < (int)parent->children.size() - 1 && parent->children[idx + 1]->keys.size() < maxkeys){ // 오른쪽 형제 노드가 여유있으면
                        node_bstar* right_sibling = parent->children[idx + 1];
                        // cur 노드에서 가장 큰거 부모노드로 올리고 오른쪽 형제노드에 부모노드에서 idx 키랑 rid 내려주고 cur 노드에서 가장 큰거 지워주기
                        right_sibling->keys.insert(right_sibling->keys.begin(), parent->keys[idx]);
                        right_sibling->rids.insert(right_sibling->rids.begin(), parent->rids[idx]);
                        parent->keys[idx] = cur->keys.back();
                        parent->rids[idx] = cur->rids.back();
                        if (!cur->is_leaf){ // 자식노드도 돌려줘야하는데 자식노드가 있으면
                            right_sibling->children.insert(right_sibling->children.begin(), cur->children.back());
                            cur->children.pop_back();
                            right_sibling->children.front()->parent = right_sibling; // 자식노드도 부모노드 바꿔주기
                        }
                        cur->keys.pop_back();
                        cur->rids.pop_back();
                        break; // split 끝났으니까 반복문 탈출
                    }
                }

                this->split_counter++;
                if (cur->parent == nullptr) {
                    // root: 1→2 split
                    node_bstar* left = cur;
                    node_bstar* right = new node_bstar(cur->is_leaf);
                    int mid = cur->keys.size() / 2;
                    int midkey = cur->keys[mid];
                    int midrid = cur->rids[mid];
                    right->keys.assign(cur->keys.begin() + mid + 1, cur->keys.end());
                    right->rids.assign(cur->rids.begin() + mid + 1, cur->rids.end());
                    left->keys.erase(left->keys.begin() + mid, left->keys.end());
                    left->rids.erase(left->rids.begin() + mid, left->rids.end());
                    if (!cur->is_leaf) {
                        right->children.assign(cur->children.begin() + mid + 1, cur->children.end());
                        cur->children.erase(cur->children.begin() + mid + 1, cur->children.end());
                        for (auto child : right->children) child->parent = right;
                    }
                    node_bstar* new_root = new node_bstar(false);
                    new_root->keys.push_back(midkey);
                    new_root->rids.push_back(midrid);
                    new_root->children.push_back(left);
                    new_root->children.push_back(right);
                    left->parent = new_root;
                    right->parent = new_root;
                    root = new_root;
                } else {
                    // non-root: 2→3 split
                    node_bstar* parent = cur->parent;
                    int cidx = find(parent->children.begin(), parent->children.end(), cur) - parent->children.begin();

                    // 오른쪽 형제 우선, 없으면 왼쪽 형제와 합치기
                    node_bstar* left_node;
                    node_bstar* right_node;
                    int sep_idx;
                    if (cidx < (int)parent->children.size() - 1) {
                        left_node = cur;
                        right_node = parent->children[cidx + 1];
                        sep_idx = cidx;
                    } else {
                        left_node = parent->children[cidx - 1];
                        right_node = cur;
                        sep_idx = cidx - 1;
                    }

                    // left + separator + right 전부 합치기
                    vector<int> all_keys, all_rids;
                    all_keys.insert(all_keys.end(), left_node->keys.begin(), left_node->keys.end());
                    all_keys.push_back(parent->keys[sep_idx]);
                    all_keys.insert(all_keys.end(), right_node->keys.begin(), right_node->keys.end());
                    all_rids.insert(all_rids.end(), left_node->rids.begin(), left_node->rids.end());
                    all_rids.push_back(parent->rids[sep_idx]);
                    all_rids.insert(all_rids.end(), right_node->rids.begin(), right_node->rids.end());

                    vector<node_bstar*> all_children;
                    if (!left_node->is_leaf) {
                        all_children.insert(all_children.end(), left_node->children.begin(), left_node->children.end());
                        all_children.insert(all_children.end(), right_node->children.begin(), right_node->children.end());
                    }

                    // 3등분: mid1, mid2 두 개를 부모로 올림
                    int n = all_keys.size(); // = 2*maxkeys + 2
                    int mid1_idx = n / 3;
                    int mid2_idx = 2 * n / 3;

                    node_bstar* middle = new node_bstar(left_node->is_leaf);
                    middle->parent = parent;

                    left_node->keys.assign(all_keys.begin(), all_keys.begin() + mid1_idx);
                    left_node->rids.assign(all_rids.begin(), all_rids.begin() + mid1_idx);
                    middle->keys.assign(all_keys.begin() + mid1_idx + 1, all_keys.begin() + mid2_idx);
                    middle->rids.assign(all_rids.begin() + mid1_idx + 1, all_rids.begin() + mid2_idx);
                    right_node->keys.assign(all_keys.begin() + mid2_idx + 1, all_keys.end());
                    right_node->rids.assign(all_rids.begin() + mid2_idx + 1, all_rids.end());

                    if (!left_node->is_leaf) {
                        left_node->children.assign(all_children.begin(), all_children.begin() + mid1_idx + 1);
                        middle->children.assign(all_children.begin() + mid1_idx + 1, all_children.begin() + mid2_idx + 1);
                        right_node->children.assign(all_children.begin() + mid2_idx + 1, all_children.end());
                        for (auto c : left_node->children) c->parent = left_node;
                        for (auto c : middle->children) c->parent = middle;
                        for (auto c : right_node->children) c->parent = right_node;
                    }

                    // 부모: sep_idx 자리를 mid1으로 교체, mid2를 그 다음에 삽입, middle 자식 추가
                    parent->keys[sep_idx] = all_keys[mid1_idx];
                    parent->rids[sep_idx] = all_rids[mid1_idx];
                    parent->keys.insert(parent->keys.begin() + sep_idx + 1, all_keys[mid2_idx]);
                    parent->rids.insert(parent->rids.begin() + sep_idx + 1, all_rids[mid2_idx]);
                    parent->children.insert(parent->children.begin() + sep_idx + 1, middle);
                }

                cur = cur->parent;
            }



        }
        
        int search(int key){
            node_bstar* cur = this->root;
            while(cur != nullptr){ // 키 찾을 때 까지
                auto it = find(cur->keys.begin(), cur->keys.end(), key);
                if(it != cur->keys.end()) return cur->rids[it - cur->keys.begin()]; // 키 찾으면 rid 반환
                int idx = lower_bound(cur->keys.begin(), cur->keys.end(), key) - cur->keys.begin(); // 같은거 없고 자식도드로 이동할 거 idx 찾기, iter - begin() 하면 idx 나옴
                if (cur->is_leaf) return -1;
                cur = cur->children[idx]; // 자식 노드로 이동
            }
            return -1; // key 못찾았을 때
        }

        pair<node_bstar*, int> find_node(int key){
            node_bstar* cur = this->root;
            while(cur != nullptr){ // 키 찾을 때 까지
                auto it = find(cur->keys.begin(), cur->keys.end(), key);
                if(it != cur->keys.end()) return make_pair(cur, (int)(it - cur->keys.begin())); // 인덱스를 반환
                int idx = lower_bound(cur->keys.begin(), cur->keys.end(), key) - cur->keys.begin(); // 같은거 없고 자식도드로 이동할 거 idx 찾기, iter - begin() 하면 idx 나옴
                if (cur->is_leaf) return make_pair(nullptr, -1);
                cur = cur->children[idx]; // 자식 노드로 이동
            }
            return make_pair(nullptr, -1); // key 못찾았을 때
        }
        
        void delete_recursive(node_bstar* root, int key){ // 재귀적으로 내려가면서 삭제해도 문제 없게 만들어주는 방식으로 구현 clrs 참고해서 구현
            // 리프노드면 그냥 날리면 된다.
            if( root->is_leaf ){ // 서브트리가 리프노드면 같은거 찾아서 날리면 끝
                auto it = find(root->keys.begin(), root->keys.end(), key);
                if (it == root->keys.end()) return; // 키 없으면 그냥 끝내기
                int idx = it - root->keys.begin();
                root->keys.erase(it);
                root->rids.erase(root->rids.begin() + idx);
                return;
            }else{ // 내부 노드이면 현재 노드에 target 있는지 없는지 확인
                auto it = find(root->keys.begin(), root->keys.end(), key);
                int idx = it - root->keys.begin();
                if (it == root->keys.end()) { // 현재 노드에 target 없을때
                    idx = lower_bound(root->keys.begin(), root->keys.end(), key) - root->keys.begin(); // 같은거 없고 자식노드로 이동할 거 idx 찾기, iter - begin() 하면 idx 나옴
                    // 자식노드로 이동하기전에 자식노드가 최소개수 이상인지 확인해야함. 부족하면 채워줘야함.
                    if (root->children[idx]->keys.size() <= (size_t)minkeys){ // 자식노드가 최소개수랑 같으면 부족한거니까 채워줘야함.
                        //왼쪽 sibling 이 여유있나 확인
                        if( idx > 0 && root->children[idx - 1]->keys.size() > minkeys){ // 왼쪽 형제 노드가 여유있으면
                            node_bstar* left_sibling = root->children[idx - 1];
                            node_bstar* child = root->children[idx]; // 시계방향으로 돌려줘야함.
                            //idx 로 내려가야하는데 idx 자손이 1개 부족하니까. 왼쪽 형제노드 key 랑 rid 부모로 올리고 형제의 children 을 가져가야함.
                            child->keys.insert(child->keys.begin(), root->keys[idx - 1]);
                            child->rids.insert(child->rids.begin(), root->rids[idx - 1]);
                            root->keys[idx - 1] = left_sibling->keys.back();
                            root->rids[idx - 1] = left_sibling->rids.back();
                            if (!left_sibling->is_leaf){ // 자식노드도 돌려줘야하는데 자식노드가 있으면
                                child->children.insert(child->children.begin(), left_sibling->children.back());
                                left_sibling->children.pop_back();
                                child->children[0]->parent = child; // 자식노드도 부모노드 바꿔주기
                            }
                            left_sibling->keys.pop_back();
                            left_sibling->rids.pop_back();

                            delete_recursive(child, key);
                        }else if (idx < root->keys.size() && root->children[idx + 1]->keys.size() > minkeys){ // 오른쪽 형제 노드가 여유있으면
                            node_bstar* right_sibling = root->children[idx + 1];
                            node_bstar* child = root->children[idx]; // 시계반대방향으로 돌려줘야함.
                            //idx 로 내려가야하는데 idx 자손이 1개 부족하니까. 오른쪽 형제노드 key 랑 rid 부모로 올리고 형제의 children 을 가져가야함.
                            child->keys.push_back(root->keys[idx]);
                            child->rids.push_back(root->rids[idx]);
                            root->keys[idx] = right_sibling->keys.front();
                            root->rids[idx] = right_sibling->rids.front();
                            if (!right_sibling->is_leaf){ // 자식노드도 돌려줘야하는데 자식노드가 있으면
                                child->children.push_back(right_sibling->children.front());
                                right_sibling->children.erase(right_sibling->children.begin());
                                child->children.back()->parent = child; // 자식노드도 부모노드 바꿔주기
                            }
                            right_sibling->keys.erase(right_sibling->keys.begin());
                            right_sibling->rids.erase(right_sibling->rids.begin());
                            delete_recursive(child, key);
     
                        }else{ // 둘다 여유 없으면 merge 해야함. idx 자식노드랑 idx + 1 자식노드 합쳐야하는데 idx 자식노드가 부족한거니까 idx + 1 자식노드로 합쳐야함.
                            if (idx == (int)root->keys.size()) idx--; // 오른쪽 형제 없으면 왼쪽 형제랑 merge
                            node_bstar* child = root->children[idx];
                            node_bstar* right_sibling = root->children[idx + 1];
                            // idx 자식노드에 부모노드에서 idx 키랑 rid 내려주고 오른쪽 형제노드 키랑 rid 다 내려주기
                            child->keys.push_back(root->keys[idx]);
                            child->rids.push_back(root->rids[idx]);
                            child->keys.insert(child->keys.end(), right_sibling->keys.begin(), right_sibling->keys.end());
                            child->rids.insert(child->rids.end(), right_sibling->rids.begin(), right_sibling->rids.end());
                            if (!child->is_leaf){ // 자식노드도 내려줘야하는데 자식노드가 있으면
                                child->children.insert(child->children.end(), right_sibling->children.begin(), right_sibling->children.end());
                                for (auto c : right_sibling->children){
                                    c->parent = child; // 자식노드도 부모노드 바꿔주기
                                }
                            }
                            // 부모노드에서 idx 키랑 rid 지워주고 오른쪽 형제노드 지워주기
                            root->keys.erase(root->keys.begin() + idx);
                            root->rids.erase(root->rids.begin() + idx);
                            root->children.erase(root->children.begin() + idx + 1);
                            delete right_sibling; // 오른쪽 형제노드 메모리 해제
                            // 재귀적으로 탐색
                            delete_recursive(child, key);
                        }
                    } else {
                        delete_recursive(root->children[idx], key); // 자식 여유 있으면 그냥 내려가기
                    }
                }else{//현재노드에 target 있을때 -> 바꿔치기 해야함.
                    // predecessor 랑 바꿔치기해야할 수 있으면 바꿔치기
                    auto left_child = root->children[idx];
                    if (left_child->keys.size() > minkeys){ // 왼쪽 자식노드가 여유있으면 왼쪽 자식노드에서 가장 큰거 가져와서 바꿔치기하고 왼쪽 서브트리에서 그거 날려버림
                        node_bstar* predecessor_node = left_child;
                        while(!predecessor_node->is_leaf){ // 왼쪽 자식노드에서 가장 큰거 찾기
                            predecessor_node = predecessor_node->children[predecessor_node->children.size() - 1];
                        }
                        int predecessor_key = predecessor_node->keys.back();
                        int predecessor_rid = predecessor_node->rids.back();
                        // 대체할 키랑 rid 가져왔으면 이제 삭제할 키랑 rid 대체할 키랑 rid로 바꿔주기
                        root->keys[idx] = predecessor_key;
                        root->rids[idx] = predecessor_rid;
                        delete_recursive(left_child, predecessor_key); // 대체한 노드에서 재귀적으로 삭제
                    }else if (idx < root->keys.size() && root->children[idx + 1]->keys.size() > minkeys){ // 오른쪽 자식노드가 여유있으면 오른쪽 자식노드에서 가장 작은거 가져와서 바꿔치기하고 오른쪽 서브트리에서 그거 날려버림
                        node_bstar* successor_node = root->children[idx + 1];
                        while(!successor_node->is_leaf){ // 오른쪽 자식노드에서 가장 작은거 찾기
                            successor_node = successor_node->children[0];
                        }
                        int successor_key = successor_node->keys.front();
                        int successor_rid = successor_node->rids.front();
                        // 대체할 키랑 rid 가져왔으면 이제 삭제할 키랑 rid 대체할 키랑 rid로 바꿔주기
                        root->keys[idx] = successor_key;
                        root->rids[idx] = successor_rid;
                        delete_recursive(root->children[idx + 1], successor_key); // 대체한 노드에서 재귀적으로 삭제
                    }else{
                        // 둘다 여유 없으면 merge 해서 하나로 만들고 그거에서 재귀적으로 삭제
                        node_bstar* child = root->children[idx];
                        node_bstar* right_sibling = root->children[idx + 1];
                        // idx 자식노드에 부모노드에서 idx 키랑 rid 내려주고 오른쪽 형제노드 키랑 rid 다 내려주기
                        child->keys.push_back(root->keys[idx]);
                        child->rids.push_back(root->rids[idx]);
                        child->keys.insert(child->keys.end(), right_sibling->keys.begin(), right_sibling->keys.end());
                        child->rids.insert(child->rids.end(), right_sibling->rids.begin(), right_sibling->rids.end());
                        if (!child->is_leaf){ // 자식노드도 내려줘야하는데 자식노드가 있으면
                            child->children.insert(child->children.end(), right_sibling->children.begin(), right_sibling->children.end());
                            for (auto c : right_sibling->children){
                                c->parent = child; // 자식노드도 부모노드 바꿔주기
                            }
                        }
                        // 부모노드에서 idx 키랑 rid 지워주고 오른쪽 형제노드 지워주기
                        root->keys.erase(root->keys.begin() + idx);
                        root->rids.erase(root->rids.begin() + idx);
                        root->children.erase(root->children.begin() + idx + 1);
                        delete right_sibling; // 오른쪽 형제노드 메모리 해제
                        // 재귀적으로 탐색
                        delete_recursive(child, key);
                    }
                }
                // left 에서 개수 충분하면 left 랑 바꿔치기하고 왼쪽 서브트리에서 그거 날려버림 : 충분하다는거는 자식 최소개수보다 1개 더 많아야함.
                // right 충분하면 right 에서 successor 가져와서 바꿔치기하고 오른쪽 서브트리에서 그거 날려버림
                // 둘다 부족하면 merge 해서 하나로 만들고 그거에서 재귀적으로 삭제

            }
        }

        bool delete_key(int key) {
            if (root == nullptr) return false;
            delete_recursive(root, key);
            if (root->keys.empty() && !root->is_leaf) {
                node_bstar* old_root = root;
                root = root->children[0];
                root->parent = nullptr;
                delete old_root;
            }
            return true;
        }

        // bool delete_key(int key){
        //     node_bstar* cur = this->root;
        //     auto result = find_node(key); // key 있는 노드랑 인덱스 찾기
        //     node_bstar* target_node = result.first;
        //     int target_index = result.second;

        //     if (target_node == nullptr) return false; // key 없으면 그냥 끝내기

        //     if (target_node->is_leaf){ //리프노드면 그냥 날리기
        //         target_node->keys.erase(target_node->keys.begin() + target_index);
        //         target_node->rids.erase(target_node->rids.begin() + target_index);
        //         node_bstar* underflow_node = target_node; // 언더플로우 노드로 삭제한 노드 설정
        //     }else{
        //         //내부 노드이면 삭제하는 키의 왼쪽 자식노드에서 가장 큰거 가져와서 대체
        //         node_bstar* left_child = target_node->children[target_index];
        //         while(!left_child->is_leaf){ // 왼쪽 자식노드에서 가장 큰거 찾기
        //             left_child = left_child->children[left_child->children.size() - 1];
        //         }
        //         int left_key = left_child->keys[left_child->keys.size() - 1];
        //         int left_rid = left_child->rids[left_child->rids.size() - 1];
        //         // 대체할 키랑 rid 가져왔으면 이제 삭제할 키랑 rid 대체할 키랑 rid로 바꿔주기
        //         target_node->keys[target_index] = left_key;
        //         target_node->rids[target_index] = left_rid;
        //         // 대체할 키랑 rid 삭제하기
        //         left_child->keys.pop_back();
        //         left_child->rids.pop_back();
        //         node_bstar* underflow_node = left_child; // 언더플로우 노드로 대체한 노드 설정
        //     }

        //     // 언더플로우 처리


        //     return true;
        // }
};