#include <vector>
#include <string>
#include <unordered_map>
#include "se_word_chain.hpp"
#include "se_word_chain_utils.hpp"
#include "se_word_chain_core.hpp"
#define MAX_BUFFER_SIZE 128

using std::vector;
using std::string;
using std::unordered_map;
using Cmap = unordered_map<char, WordMapElement>;
using CCmap = unordered_map<char, unordered_map<char, WordMapElement> >;
using Dmap = unordered_map<char, int>;
SearchInterface::~SearchInterface() {}

se_errcode NaiveSearch::DfsSearch(char cur_head) {
    SE_WORD_CHAIN_LOG("cur head:%c", cur_head);
    auto iter_h = m_wmap.find(cur_head);
    if (iter_h == m_wmap.end()) {
        return SE_OK;
    }

    TWmap tail_map = iter_h->second;
    int count = 0;
    for (auto iter_t = tail_map.begin(); iter_t != tail_map.end(); ++iter_t) {
        auto& item_word = m_wmap[cur_head][iter_t->first];
        if (item_word.GetVisitFlag()) {
            continue;
        }
        auto& begin_item_dist = m_dmap[m_begin_item][iter_t->first];
        m_cur_search_chain.push_back(item_word.GetLongestWord());
        SE_WORD_CHAIN_LOG("cur len:%d", item_word.GetLongestLen());
        SE_WORD_CHAIN_LOG("cur dist:%d", begin_item_dist.GetDistance());
        int sum_dist;
        switch (m_type) {
            case word_longest: {
                sum_dist = 1 + m_cur_search_len;
                m_cur_search_len += 1;
                break;
            }
            case letter_longest: {
                sum_dist = item_word.GetLongestLen() + m_cur_search_len;
                m_cur_search_len += item_word.GetLongestLen();
                break;
            }
        }
        if (sum_dist > begin_item_dist.GetDistance()) {
            SE_WORD_CHAIN_LOG("modify:%c===>%c, origin:%d, now:%d", m_begin_item, iter_t->first, begin_item_dist.GetDistance(), sum_dist);
            begin_item_dist.SetDistance(sum_dist);
            begin_item_dist.SetWordChain(m_cur_search_chain);
            SE_WORD_CHAIN_LOG("after modify:%c===>%c, origin:%d, now:%d", m_begin_item, iter_t->first, begin_item_dist.GetDistance(), sum_dist);
        }
        item_word.SetVisitFlag(true);
        DfsSearch(iter_t->first);
        item_word.SetVisitFlag(false);
        switch (m_type) {
            case word_longest: {
                m_cur_search_len -= 1;
                break;
            }
            case letter_longest: {
                m_cur_search_len -= item_word.GetLongestLen();
            }
        }
        m_cur_search_chain.pop_back();
    }
    SE_WORD_CHAIN_LOG("end head:%c", cur_head);
    //PrintMap<DistanceElement>(m_dmap);
    return SE_OK;
}

se_errcode NaiveSearch::Search() {
	set<char> word_head_set;
    set<char> word_tail_set;
    for (auto iter_h = m_wmap.begin(); iter_h != m_wmap.end(); ++iter_h) {
        char head = iter_h->first;
        word_head_set.insert(head);
        for (auto iter_t = iter_h->second.begin(); iter_t != iter_h->second.end(); ++iter_t) {
            char tail = iter_t->first;
            word_tail_set.insert(tail);
        }
    }
    vector<char> head_vector(word_head_set.size());
    //auto iter_d = std::set_difference(word_head_set.begin(), word_head_set.end(), word_tail_set.begin(), word_tail_set.end(), head_vector.begin());
    //head_vector.resize(iter_d - head_vector.begin());
    head_vector.assign(word_head_set.begin(), word_head_set.end());
    for (auto iter = head_vector.begin(); iter != head_vector.end(); ++iter) {
        m_begin_item = *iter;
        DfsSearch(*iter);
    } 
	
 
    return SE_OK;
}

se_errcode NaiveSearch::Search(const char& head) {
    m_begin_item = head;
    DfsSearch(head);
    return SE_OK;
}

se_errcode NaiveSearch::LookUp(vector<string>& output_buffer, const char& head, const char& tail, WordChainError& handle_error) const {
    if (DEBUG) {
        PrintMap<DistanceElement>(m_dmap);
    }
    se_errcode ret = SE_OK;
    char longest_head = NO_ASSIGN_HEAD;
    char longest_tail = NO_ASSIGN_TAIL;
    int temp_head_longest = 0;

    if (head == NO_ASSIGN_HEAD) {
        for (auto iter_h = m_dmap.begin(); iter_h != m_dmap.end(); ++iter_h) {
            auto tail_map = iter_h->second;
            if (tail == NO_ASSIGN_TAIL) {
                //printf("head look:%c\n", iter_h->first);
                int temp_tail_longest = 0;
                char longest_tail_temp = NO_ASSIGN_TAIL;
                for (auto iter_t = tail_map.begin(); iter_t != tail_map.end(); ++iter_t) {
                    if (iter_t->second.GetDistance() > temp_tail_longest) {
                        temp_tail_longest = iter_t->second.GetDistance();
                        longest_tail_temp = iter_t->first;
                    }
                }
                //printf("temp_tail_longest:%d\n", temp_tail_longest);
                //printf("temp_head_longest:%d\n", temp_head_longest);
                if (temp_tail_longest > temp_head_longest) {
                    temp_head_longest = temp_tail_longest;
                    longest_head = iter_h->first;
                    longest_tail = longest_tail_temp;
                }
            } else {
                auto element = tail_map.find(tail)->second;
                auto dist = element.GetDistance();
                if (dist != 0) {
                    if (dist > temp_head_longest) {
                        temp_head_longest = dist;
                        longest_head = iter_h->first;
                        longest_tail = tail;
                    }
                }
            }
        }
    } else {
        if (tail == NO_ASSIGN_TAIL) {
            longest_head = head;
            auto tail_map_iter = m_dmap.find(head);
            auto tail_map = tail_map_iter->second;
           for (auto iter_t = tail_map.begin(); iter_t != tail_map.end(); ++iter_t) {
                if (iter_t->second.GetDistance() > temp_head_longest) {
                    temp_head_longest = iter_t->second.GetDistance();
                    longest_tail = iter_t->first;
                }
            }
        } else {
            auto tail_find_iter = m_dmap.find(head);
            auto element = tail_find_iter->second.find(tail)->second;
            auto dist = element.GetDistance();
            if (dist != 0) {
                longest_head = head;
                longest_tail = tail;
                temp_head_longest = dist;
            }
        }
    }

    if (temp_head_longest == 0) {
        char buffer[MAX_BUFFER_SIZE];
        sprintf(buffer, "no available word chain for head(%c) and tail(%c)\n", head ? head : '0' , tail ? tail : '0');
        string error_content(buffer);
        int error_code = SE_NO_AVAILABLE_WORD_CHAIN;
        handle_error.AppendInfo(error_code, error_content);
        return SE_NO_AVAILABLE_WORD_CHAIN;
    } else {
        auto tail_map_iter = m_dmap.find(longest_head);
        auto element_iter = tail_map_iter->second.find(longest_tail);
        element_iter->second.CopyWordBuffer(output_buffer);
        return SE_OK;
    }
}

string tolower(string str){
    for(int i = 0; i < str.length(); i++)
        str[i] = tolower(str[i]);
    return str;
}

se_errcode ExtractWord(const string& input_text, vector<string>& input_buffer, WordChainError& handle_error) {
    /*
     * Brief:
     *     extract all the correct word in the input, transform it into lower case letters and store it in input buffer
     *     all the word in the input_text are seperated with non-alpha character
     * Param:
     *     input_text: input text including word
     *     input_buffer: a vector including all the correctly seperated word
     * Return:
     *     all: SE_OK
     */
    int len=input_text.length();
    for(int i=0;i<len;i++){
        if(isalpha(input_text[i])){
            for(int j=i;j<len;j++){
                if(!isalpha(input_text[j])){
                    input_buffer.push_back(tolower(input_text.substr(i,j-i)));
                    i=j;
                    break;
                }
				if (j == len - 1) {
					input_buffer.push_back(tolower(input_text.substr(i, j - i+1)));
					i = j;
					break;
				}
            }
        }
    }
    return SE_OK;
}

se_errcode GenerateWordMap(const vector<string>& input_buffer, unordered_map<char, unordered_map<char, WordMapElement> >& origin_word_map, WordChainError& handle_error) {
    se_errcode ret = SE_OK;

    for (auto iter = input_buffer.begin(); iter != input_buffer.end(); ++iter) {
        string item = *iter;
        Word word_item = Word(item);
        auto head_find_flag = origin_word_map.find(word_item.GetHead());
        if (head_find_flag == origin_word_map.end()) {
            Cmap tail_map;
            tail_map.insert(Cmap::value_type(word_item.GetTail(), WordMapElement(word_item)));
            origin_word_map.insert(CCmap::value_type(word_item.GetHead(), tail_map));
        } else {
            auto& tail_map = head_find_flag->second;
            auto tail_find_flag =  tail_map.find(word_item.GetTail());
            if (tail_find_flag == tail_map.end()) {
                tail_map.insert(Cmap::value_type(word_item.GetTail(), WordMapElement(word_item)));
            } else {
                if ((ret = tail_find_flag->second.AppendWord(word_item.GetWord())) != SE_OK) {
                    char buffer[MAX_BUFFER_SIZE];
                    sprintf(buffer, "repeat word: %s\n", word_item.GetWord().c_str());
                    string error_content(buffer);
                    int error_code = SE_NO_AVAILABLE_WORD_CHAIN;
                    handle_error.AppendInfo(error_code, error_content);
                }
            }
        }
    }
    return ret;
}

se_errcode CheckCircle(const unordered_map<char, unordered_map<char, WordMapElement> >& origin_word_map, bool& has_circle, WordChainError& handle_error) {
    /*
     * Brief:
     *     check whether there is circle in map
     * Param:
     *     origin_word_map: word map established by head and tail letter(<head, <tail, element>)
     *     has_circle: whether has circle in map
     * Return:
     *     if has circle, return true, otherwise return false
     */
    
    Dmap indegree;
    //generate indegree map
    for(int i=0;i<26;i++){
        char target = i+'a';
        auto iter=origin_word_map.find(target);
        if(iter==origin_word_map.end()){
            indegree.insert(Dmap::value_type(target,0));
        }else{
            int size=iter->second.size();
            indegree.insert(Dmap::value_type(target,size));
        }
    }
    bool change=true;
    //delete points which have 0-indegree
    //update related points' indegree
    while(change){
        change=false;
        for(int i=0;i<26;i++){
            char target = i+'a';
            auto iter=indegree.find(target);
            if(iter->second==0){
                change=true;
                iter->second=-1;
                for(int j=0;j<26;j++){
                    char key=j+'a';
                    auto temp=origin_word_map.find(key);
                    if (temp == origin_word_map.end()) {
                        continue;
                    } else {
                        auto iter_temp = temp->second;
                        if (iter_temp.find(target) != iter_temp.end()) {
                             indegree.find(key)->second -= 1;
                        }
                    }
                }
            }
        }
    }
	//release self circle
	for (int i = 0; i < 26; i++) {
		char target = i + 'a';
		auto iter = origin_word_map.find(target);
		if (iter != origin_word_map.end()) {
			auto iter_temp = iter->second.find(target);
			if (iter_temp != iter->second.end()) {
				auto set = iter_temp->second;
				if (set.GetSetSize() == 1) {
					indegree.find(target)->second -= 1;
				}
			}
		}
	}
    //if there is a point cannot be deleted,the graph has a loop
    has_circle=false;
    for(auto iter=indegree.begin();iter!=indegree.end();iter++){
        if(iter->second>0){
            has_circle=true;
            break;
        }
    }
    return SE_OK;
}

se_errcode ChainSearch(const unordered_map<char, unordered_map<char, WordMapElement> >& origin_word_map, vector<string>& output_buffer, const LongestWordChainType& longest_type, const char& head, const char& tail, WordChainError& handle_error) {
    int ret = SE_OK;
    SearchInterface* handle_search = NULL;
    int search_type = 0;
    switch (search_type) {
        case 0: {
            handle_search = new NaiveSearch(origin_word_map, longest_type);
            if (head == NO_ASSIGN_HEAD) {
                handle_search->Search();
            } else {
                NaiveSearch *h = dynamic_cast<NaiveSearch*>(handle_search);
                h->Search(head);
            }
            break;
        }   
        default: {
            char buffer[MAX_BUFFER_SIZE];
            sprintf(buffer, "not support search algorithm\n");
            string error_content(buffer);
            int error_code = SE_NO_AVAILABLE_WORD_CHAIN;
            handle_error.AppendInfo(error_code, error_content);
        }
    }
    ret = handle_search->LookUp(output_buffer, head, tail, handle_error);
    delete(handle_search);
    return ret;
}

se_errcode CalculateLongestChain(const vector<string>& input_buffer, vector<string>& output_buffer, const LongestWordChainType& longest_type, const char& head, const char& tail, bool enable_circle, WordChainError& handle_error) {
    CCmap origin_word_map;
    se_errcode ret = SE_OK;
    // generate word map
    ret = GenerateWordMap(input_buffer, origin_word_map, handle_error);
    switch (ret) {
        case SE_REPEAT_WORD: ret = SE_OK;// fall through
        case SE_OK: break;
        default: return ret;
    }
    if (DEBUG) {
        PrintMap<WordMapElement>(origin_word_map);
    }
    // check circle
    bool has_circle;
    CheckCircle(origin_word_map, has_circle, handle_error);
    if (!enable_circle && has_circle) {
		char buffer[MAX_BUFFER_SIZE];
		sprintf(buffer, "Error Type: input has circle but not enable circle\n");
		string error_content(buffer);
		int error_code = SE_HAS_CIRCLE;
		handle_error.AppendInfo(error_code, error_content);
        return SE_HAS_CIRCLE;
    }

    // calculate longest chain
    switch (longest_type) {
        case word_longest:
            // fall through
        case letter_longest: {
            ret = ChainSearch(origin_word_map, output_buffer, longest_type, head, tail, handle_error);
            break;
        }
        default: {
            char buffer[MAX_BUFFER_SIZE];
            sprintf(buffer, "invalid longest_type argument: %d\n", longest_type);
            string error_content(buffer);
            int error_code = SE_NO_AVAILABLE_WORD_CHAIN;
            handle_error.AppendInfo(error_code, error_content);
            return SE_INVALID_LONGEST_TYPE;
        }
    }
    return ret;
}

se_errcode OutputTransform(const vector<string>& output_buffer, string& output_text, WordChainError& handle_error) {
    /*
     * Brief:
     *     transform word into output text
     *     all the word in output_text need to be seperated with line break
     * Param:
     *     output_buffer: a buffer including all the correct word to be output
     *     output_text: a string including all the word in correct form
     * Return:
     *     all: SE_OK
     */
    int size=output_buffer.size();
    for(int i=0;i<size;i++){
        output_text+=output_buffer[i]+"\n";
    }
    return SE_OK;
}

se_errcode Calculate(const string& input_text, string& output_text, LongestWordChainType& longest_type, const char& head, const char& tail, bool enable_circle, WordChainError& handle_error) {
    int ret = SE_OK;
    vector<string> input_buffer;
    vector<string> output_buffer;
    if ((ret = ExtractWord(input_text, input_buffer, handle_error)) != SE_OK) {
        goto ERROR_CAL;
    }

    if ((ret = CalculateLongestChain(input_buffer, output_buffer, longest_type, head, tail, enable_circle, handle_error)) != SE_OK) {
        goto ERROR_CAL;
    }

    if ((ret = OutputTransform(output_buffer, output_text, handle_error)) != SE_OK) {
        goto ERROR_CAL;
    }
    return SE_OK;
    
ERROR_CAL:
    return ret;
}

se_errcode WordMapElement::AppendWord(const string& word) {
    int size = word.size();
    auto iter = m_word_set.begin();
    for (; iter != m_word_set.end(); ++iter) {
        if (iter->data == word) {
            return SE_REPEAT_WORD;
        }
        if (iter->size < size) {
            break;
        }
    }
    m_word_set.insert(iter, WordElement(word));

    return SE_OK;
}

string WordMapElement::ToString() const {
    string ret = "";
    for (auto iter = m_word_set.begin(); iter != m_word_set.end(); ++iter) {
        ret += (*iter).data;
    }
    return ret;
}

string DistanceElement::ToString() const {
    string ret = "";
    for (auto iter = m_word_buffer.begin(); iter != m_word_buffer.end(); ++iter) {
        if (ret == "") {
            ret += *iter;
        } else {
            ret += ("-" + *iter);
        }
    }
    return ret;
}

void DistanceElement::SetWordChain(const vector<string>& others) {
    int size = others.size();
    m_word_buffer.assign(others.begin(), others.end());
    m_word_buffer.resize(size);
}

template<class T> void PrintMap(const unordered_map<char, unordered_map<char, T> >& input_map) {
    for (auto iter1 = input_map.begin(); iter1 != input_map.end(); ++iter1) {
        //fprintf(stdout, "-------------------------------------\n");
        unordered_map<char, T> item = iter1->second;
        for (auto iter2 = item.begin(); iter2 != item.end(); ++iter2) {
            string str = iter2->second.ToString().c_str();
            if (str != "") {
                fprintf(stdout, "%c===>%c(%s)\n", iter1->first, iter2->first, str.c_str());
            }
        }
        //fprintf(stdout, "-------------------------------------\n");
    }
}
