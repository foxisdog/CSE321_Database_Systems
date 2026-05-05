#include <vector>
#include <iostream>
#include <algorithm>
using namespace std;

class node_bplus{
    public:
        node_bplus* parent;
        node_bplus* next; // 리프 노드끼리 연결하는 포인터
        vector<node_bplus*> children; // 자식 노드 가리키는 포인터 벡터
        vector<int> rids; // rid (리프 노드에만 있음)
        vector<int> keys; // key
        bool is_leaf; // 끝인지 아닌지

        node_bplus(bool is_leaf){ this->is_leaf = is_leaf; parent = nullptr; next = nullptr; }
};

class bplus_tree{
    private:
        node_bplus* root;
        int split_counter = 0; // split 횟수 세는 변수
        int order;
        int maxkeys;
        int minkeys;
    public:
        bplus_tree(int order){ this->order = order; root = nullptr; maxkeys = order - 1; minkeys = (order + 1) / 2 - 1;}
        int get_split_count() { return split_counter; }
        node_bplus* get_root() { return root; }

        void insert(int key, int rid){
            if (root == nullptr){ // 트리가 없으면 트리 만들어야함
                root = new node_bplus(true);
                root->keys.push_back(key);
                root->rids.push_back(rid);
                return;
            }
            // 트리는 있고 이제 진짜 삽입하는경우
            // B+ tree 는 내부 노드에 separator 있으니까 upper_bound 써야함
            node_bplus* cur = root;
            while(!cur->is_leaf){
                int idx = upper_bound(cur->keys.begin(), cur->keys.end(), key) - cur->keys.begin();
                cur = cur->children[idx];
            }
            // 넣어야하는 리프노드 도착했음
            auto it = lower_bound(cur->keys.begin(), cur->keys.end(), key);
            int idx = it - cur->keys.begin();
            cur->keys.insert(it, key);
            cur->rids.insert(cur->rids.begin() + idx, rid); // 둘다 삽입함.

            // split 하고 부모노드 올리는거 반복문으로
            while(cur->keys.size() > maxkeys){ // 스플릿 해야하는 경우
                this->split_counter++;
                node_bplus* left = cur; // left 는 기존노드 재사용
                node_bplus* right = new node_bplus(cur->is_leaf); // right 는 다시 만들고
                int push_key; // 부모로 올릴 key

                if(cur->is_leaf){ // 리프 노드 split : copy-up (mid 키가 right 리프에 남음)
                    int mid = cur->keys.size() / 2;
                    right->keys.assign(cur->keys.begin() + mid, cur->keys.end());
                    right->rids.assign(cur->rids.begin() + mid, cur->rids.end());
                    left->keys.erase(left->keys.begin() + mid, left->keys.end());
                    left->rids.erase(left->rids.begin() + mid, left->rids.end());
                    right->next = left->next; // 링크드 리스트 연결
                    left->next = right;
                    push_key = right->keys.front(); // 오른쪽 리프 첫번째 키가 부모로 올라감
                }else{ // 내부 노드 split : push-up (mid 키가 부모로 올라가고 노드에서 없어짐)
                    int mid = cur->keys.size() / 2;
                    push_key = cur->keys[mid];
                    right->keys.assign(cur->keys.begin() + mid + 1, cur->keys.end());
                    right->children.assign(cur->children.begin() + mid + 1, cur->children.end());
                    left->keys.erase(left->keys.begin() + mid, left->keys.end());
                    left->children.erase(left->children.begin() + mid + 1, left->children.end());
                    for(auto child : right->children) child->parent = right; // 자식노드도 부모노드 바꿔주기
                }

                if(cur->parent == nullptr){ // 루트면 새 루트 만들기
                    node_bplus* new_root = new node_bplus(false);
                    new_root->keys.push_back(push_key);
                    new_root->children.push_back(left);
                    new_root->children.push_back(right);
                    left->parent = new_root;
                    right->parent = new_root;
                    root = new_root;
                    break;
                }else{ // 부모 있으면 부모에 push_key 올리기
                    node_bplus* parent = cur->parent;
                    auto pit = lower_bound(parent->keys.begin(), parent->keys.end(), push_key);
                    int pidx = pit - parent->keys.begin();
                    parent->keys.insert(pit, push_key);
                    parent->children.insert(parent->children.begin() + pidx + 1, right);
                    right->parent = parent;
                    cur = parent; // 부모로 올라가서 오버플로우 확인
                }
            }
        }

        int search(int key){
            if(root == nullptr) return -1;
            node_bplus* cur = root;
            while(!cur->is_leaf){ // 리프 노드까지 내려가기
                int idx = upper_bound(cur->keys.begin(), cur->keys.end(), key) - cur->keys.begin();
                cur = cur->children[idx];
            }
            // 리프 노드에서 키 찾기
            auto it = lower_bound(cur->keys.begin(), cur->keys.end(), key);
            if(it == cur->keys.end() || *it != key) return -1;
            return cur->rids[it - cur->keys.begin()];
        }

        // lo ~ hi 범위 검색 : 링크드 리스트로 순차 스캔하면 됨
        vector<pair<int,int>> range_search(int lo, int hi){
            vector<pair<int,int>> result;
            if(root == nullptr) return result;
            node_bplus* cur = root;
            while(!cur->is_leaf){ // lo 있을 리프 노드까지 내려가기
                int idx = upper_bound(cur->keys.begin(), cur->keys.end(), lo) - cur->keys.begin();
                cur = cur->children[idx];
            }
            // next 포인터로 순차 스캔
            while(cur != nullptr){
                for(int i = 0; i < (int)cur->keys.size(); i++){
                    if(cur->keys[i] > hi) return result;
                    if(cur->keys[i] >= lo)
                        result.push_back({cur->keys[i], cur->rids[i]});
                }
                cur = cur->next;
            }
            return result;
        }

        void fix_underflow(node_bplus* node){
            if(node == root){ // 루트가 빈 내부 노드면 자식이 새 루트
                if(!node->is_leaf && node->keys.empty()){
                    root = node->children[0];
                    root->parent = nullptr;
                    delete node;
                }
                return;
            }
            if((int)node->keys.size() >= minkeys) return; // 언더플로우 아니면 끝

            node_bplus* parent = node->parent;
            int cidx = find(parent->children.begin(), parent->children.end(), node) - parent->children.begin();

            if(node->is_leaf){ // 리프 노드 언더플로우 처리
                // 왼쪽 형제에서 빌리기
                if(cidx > 0){
                    node_bplus* left = parent->children[cidx - 1];
                    if((int)left->keys.size() > minkeys){
                        node->keys.insert(node->keys.begin(), left->keys.back());
                        node->rids.insert(node->rids.begin(), left->rids.back());
                        left->keys.pop_back();
                        left->rids.pop_back();
                        parent->keys[cidx - 1] = node->keys.front(); // separator 갱신
                        return;
                    }
                }
                // 오른쪽 형제에서 빌리기
                if(cidx < (int)parent->children.size() - 1){
                    node_bplus* right = parent->children[cidx + 1];
                    if((int)right->keys.size() > minkeys){
                        node->keys.push_back(right->keys.front());
                        node->rids.push_back(right->rids.front());
                        right->keys.erase(right->keys.begin());
                        right->rids.erase(right->rids.begin());
                        parent->keys[cidx] = right->keys.front(); // separator 갱신
                        return;
                    }
                }
                // 둘다 여유 없으면 merge 해야함
                if(cidx > 0){ // 왼쪽 형제랑 merge
                    node_bplus* left = parent->children[cidx - 1];
                    left->keys.insert(left->keys.end(), node->keys.begin(), node->keys.end());
                    left->rids.insert(left->rids.end(), node->rids.begin(), node->rids.end());
                    left->next = node->next; // 링크드 리스트 연결
                    parent->keys.erase(parent->keys.begin() + cidx - 1);
                    parent->children.erase(parent->children.begin() + cidx);
                    delete node;
                }else{ // 오른쪽 형제랑 merge
                    node_bplus* right = parent->children[cidx + 1];
                    node->keys.insert(node->keys.end(), right->keys.begin(), right->keys.end());
                    node->rids.insert(node->rids.end(), right->rids.begin(), right->rids.end());
                    node->next = right->next;
                    parent->keys.erase(parent->keys.begin() + cidx);
                    parent->children.erase(parent->children.begin() + cidx + 1);
                    delete right;
                }
            }else{ // 내부 노드 언더플로우 처리
                // 왼쪽 형제에서 빌리기 : 부모 separator 내려오고, 형제 마지막 key 올라감
                if(cidx > 0){
                    node_bplus* left = parent->children[cidx - 1];
                    if((int)left->keys.size() > minkeys){
                        node->keys.insert(node->keys.begin(), parent->keys[cidx - 1]);
                        parent->keys[cidx - 1] = left->keys.back();
                        left->keys.pop_back();
                        node->children.insert(node->children.begin(), left->children.back());
                        left->children.pop_back();
                        node->children.front()->parent = node; // 자식노드도 부모노드 바꿔주기
                        return;
                    }
                }
                // 오른쪽 형제에서 빌리기
                if(cidx < (int)parent->children.size() - 1){
                    node_bplus* right = parent->children[cidx + 1];
                    if((int)right->keys.size() > minkeys){
                        node->keys.push_back(parent->keys[cidx]);
                        parent->keys[cidx] = right->keys.front();
                        right->keys.erase(right->keys.begin());
                        node->children.push_back(right->children.front());
                        right->children.erase(right->children.begin());
                        node->children.back()->parent = node; // 자식노드도 부모노드 바꿔주기
                        return;
                    }
                }
                // 둘다 여유 없으면 merge : 부모 separator 내려서 합침
                if(cidx > 0){ // 왼쪽 형제랑 merge
                    node_bplus* left = parent->children[cidx - 1];
                    left->keys.push_back(parent->keys[cidx - 1]); // separator 내려오기
                    left->keys.insert(left->keys.end(), node->keys.begin(), node->keys.end());
                    left->children.insert(left->children.end(), node->children.begin(), node->children.end());
                    for(auto c : node->children) c->parent = left; // 자식노드도 부모노드 바꿔주기
                    parent->keys.erase(parent->keys.begin() + cidx - 1);
                    parent->children.erase(parent->children.begin() + cidx);
                    delete node;
                }else{ // 오른쪽 형제랑 merge
                    node_bplus* right = parent->children[cidx + 1];
                    node->keys.push_back(parent->keys[cidx]); // separator 내려오기
                    node->keys.insert(node->keys.end(), right->keys.begin(), right->keys.end());
                    node->children.insert(node->children.end(), right->children.begin(), right->children.end());
                    for(auto c : right->children) c->parent = node; // 자식노드도 부모노드 바꿔주기
                    parent->keys.erase(parent->keys.begin() + cidx);
                    parent->children.erase(parent->children.begin() + cidx + 1);
                    delete right;
                }
            }

            fix_underflow(parent); // 부모도 언더플로우 확인
        }

        bool delete_key(int key){
            if(root == nullptr) return false;
            node_bplus* cur = root;
            while(!cur->is_leaf){ // 리프 노드까지 내려가기
                int idx = upper_bound(cur->keys.begin(), cur->keys.end(), key) - cur->keys.begin();
                cur = cur->children[idx];
            }
            // 리프에서 삭제
            auto it = lower_bound(cur->keys.begin(), cur->keys.end(), key);
            if(it == cur->keys.end() || *it != key) return false;
            int idx = it - cur->keys.begin();
            cur->keys.erase(it);
            cur->rids.erase(cur->rids.begin() + idx);
            fix_underflow(cur);
            return true;
        }
};
