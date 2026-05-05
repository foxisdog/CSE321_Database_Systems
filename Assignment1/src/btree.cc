#include <vector>
#include <iostream>
#include <algorithm>
using namespace std;

class node_btree{
    public:
        node_btree* parent;
        vector<node_btree*> children; // 자식 노드 가리키는 포인터 벡터
        vector<int> rids; // rid
        vector<int> keys; // key
        // key 랑 rid 랑 1:1 로 같은거임
        bool is_leaf; // 끝인지 아닌지

        node_btree(bool is_leaf){ this->is_leaf = is_leaf; parent = nullptr; }
};

class btree{
    private:
        node_btree* root;
        int split_counter = 0; // split 횟수 세는 변수
        int order;
        int maxkeys;
        int minkeys;
    public:
        btree(int order){ this->order = order; root = nullptr; maxkeys = order - 1; minkeys = (order + 1) / 2 - 1;}
        int get_split_count() { return split_counter; }
        node_btree* get_root() { return root; }
        void insert(int key, int rid){
            if (root == nullptr){ // 트리가 없으면 트리 만들어야함}
                root = new node_btree(true);
                root->keys.push_back(key);
                root->rids.push_back(rid);
                return;
            }
            // 트리는 있고 이제 진짜 삽입하는경우
            node_btree* cur = root;
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
                this->split_counter++; // split 횟수 증가
                node_btree* left = cur; // left 는 기존노드 재사용
                node_btree* right = new node_btree(cur->is_leaf); // right 는 다시 만들고
                int mid = cur->keys.size() / 2; // mid 는 가운데 인덱스
                int midkey = cur->keys[mid]; // midkey 는 가운데 키
                int midrid = cur->rids[mid]; // midrid 는 가운데 rid // left, mid ,right 만들었음
                // right 노드에 mid 뒤에 있는 키랑 rid 옮겨주기
                right->keys.assign(cur->keys.begin() + mid + 1, cur->keys.end());
                right->rids.assign(cur->rids.begin() + mid + 1, cur->rids.end());
                // left 노드에는 mid 뒤에 있는거 다 지워주기
                left->keys.erase(left->keys.begin() + mid, left->keys.end());
                left->rids.erase(left->rids.begin() + mid, left->rids.end());
                // right 노드가 자식노드도 가지고 있으면 자식노드 도 복사
                if (!cur->is_leaf){
                    right->children.assign(cur->children.begin() + mid + 1, cur->children.end());
                    cur->children.erase(cur->children.begin() + mid + 1, cur->children.end());
                    // 자식노드들도 부모노드 바꿔주기
                    for (auto child : right->children){
                        child->parent = right;
                    }
                }
                // 이제 mid 는 올려
                //부모 없을때랑 있을때랑 나눠서
                if(cur->parent == nullptr){
                    node_btree* new_root = new node_btree(0);
                    new_root->keys.push_back(midkey);
                    new_root->rids.push_back(midrid);
                    new_root->children.push_back(left);
                    new_root->children.push_back(right);
                    left->parent = new_root;
                    right->parent = new_root;
                    root = new_root;
                }else{// 부모 노드 있을떄
                    node_btree* parent = cur->parent;
                    auto it = lower_bound(parent->keys.begin(), parent->keys.end(), midkey);
                    int idx = it - parent->keys.begin();
                    parent->keys.insert(it, midkey);
                    parent->rids.insert(parent->rids.begin() + idx, midrid);
                    parent->children.insert(parent->children.begin() + idx + 1, right); // 오른쪽 자식노드로 추가
                    right->parent = parent; // right 노드 부모노드로 연결
                }

                cur = cur->parent; // 부모노드로 올라감
            }



        }
        
        int search(int key){
            node_btree* cur = this->root;
            while(cur != nullptr){ // 키 찾을 때 까지
                auto it = find(cur->keys.begin(), cur->keys.end(), key);
                if(it != cur->keys.end()) return cur->rids[it - cur->keys.begin()]; // 키 찾으면 rid 반환
                int idx = lower_bound(cur->keys.begin(), cur->keys.end(), key) - cur->keys.begin(); // 같은거 없고 자식도드로 이동할 거 idx 찾기, iter - begin() 하면 idx 나옴
                if (cur->is_leaf) return -1;
                cur = cur->children[idx]; // 자식 노드로 이동
            }
            return -1; // key 못찾았을 때
        }

        pair<node_btree*, int> find_node(int key){
            node_btree* cur = this->root;
            while(cur != nullptr){ // 키 찾을 때 까지
                auto it = find(cur->keys.begin(), cur->keys.end(), key);
                if(it != cur->keys.end()) return make_pair(cur, (int)(it - cur->keys.begin())); // 인덱스를 반환
                int idx = lower_bound(cur->keys.begin(), cur->keys.end(), key) - cur->keys.begin(); // 같은거 없고 자식도드로 이동할 거 idx 찾기, iter - begin() 하면 idx 나옴
                if (cur->is_leaf) return make_pair(nullptr, -1);
                cur = cur->children[idx]; // 자식 노드로 이동
            }
            return make_pair(nullptr, -1); // key 못찾았을 때
        }
        
        void delete_recursive(node_btree* root, int key){ // 재귀적으로 내려가면서 삭제해도 문제 없게 만들어주는 방식으로 구현 clrs 참고해서 구현
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
                            node_btree* left_sibling = root->children[idx - 1];
                            node_btree* child = root->children[idx]; // 시계방향으로 돌려줘야함.
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
                            node_btree* right_sibling = root->children[idx + 1];
                            node_btree* child = root->children[idx]; // 시계반대방향으로 돌려줘야함.
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
                            node_btree* child = root->children[idx];
                            node_btree* right_sibling = root->children[idx + 1];
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
                        node_btree* predecessor_node = left_child;
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
                        node_btree* successor_node = root->children[idx + 1];
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
                        node_btree* child = root->children[idx];
                        node_btree* right_sibling = root->children[idx + 1];
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
                node_btree* old_root = root;
                root = root->children[0];
                root->parent = nullptr;
                delete old_root;
            }
            return true;
        }

        // bool delete_key(int key){
        //     node_btree* cur = this->root;
        //     auto result = find_node(key); // key 있는 노드랑 인덱스 찾기
        //     node_btree* target_node = result.first;
        //     int target_index = result.second;

        //     if (target_node == nullptr) return false; // key 없으면 그냥 끝내기

        //     if (target_node->is_leaf){ //리프노드면 그냥 날리기
        //         target_node->keys.erase(target_node->keys.begin() + target_index);
        //         target_node->rids.erase(target_node->rids.begin() + target_index);
        //         node_btree* underflow_node = target_node; // 언더플로우 노드로 삭제한 노드 설정
        //     }else{
        //         //내부 노드이면 삭제하는 키의 왼쪽 자식노드에서 가장 큰거 가져와서 대체
        //         node_btree* left_child = target_node->children[target_index];
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
        //         node_btree* underflow_node = left_child; // 언더플로우 노드로 대체한 노드 설정
        //     }

        //     // 언더플로우 처리


        //     return true;
        // }
};