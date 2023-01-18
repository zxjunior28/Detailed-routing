#ifndef SRC_ROUTING_INPUT_HPP_
#define SRC_ROUTING_INPUT_HPP_


#include <routing/system.hpp>

namespace routing {

class Input {
 public:
    using system_ptr_type = std::unique_ptr<backend::System>;
    using string_type = std::string;

    /* --- Constructor & Destructor --- */
    explicit Input(std::ifstream& in) : in_(in) {
        if (in.fail())
            std::cerr << "no such file!! " <<  std::endl;
        else
            system_ptr_ = std::make_unique<backend::System>();
    }
    virtual ~Input() = default;
    /*----------------------------------*/

    /*read input file to get layout info*/
    system_ptr_type readFile(void);


 private:
    system_ptr_type system_ptr_{nullptr};
    std::ifstream& in_;
};

Input::system_ptr_type Input::readFile() {
    auto& columns = system_ptr_->columns;
    auto& nets = system_ptr_->nets;
    auto& num_columns = system_ptr_->num_columns;
    auto& density = system_ptr_->density;

    std::string s1, s2;
    // 1st layer
    std::getline(in_, s1);
    std::getline(in_, s2);
    std::stringstream ss1(s1);
    std::stringstream ss2(s2);

    int top, bottom;
    int num = 0;
    while (ss1 >> top) {
        ss2 >> bottom;
        columns.push_back(backend::Column{top, bottom});

        if (top != 0) {
            nets[top].left = (nets[top].left == -1) ? num : nets[top].left;
            nets[top].right = std::max(nets[top].right, num);
            nets[top].length = nets[top].right - nets[top].left;
        }

        if (bottom != 0) {
            nets[bottom].left = (nets[bottom].left == -1) ? num : nets[bottom].left;
            nets[bottom].right = std::max(nets[bottom].right, num);
            nets[bottom].length = nets[bottom].right - nets[bottom].left;
        }

        num++;
    }

    num_columns = columns.size();

    std::vector<int> overlap(num_columns, 0);
    for (auto it = nets.begin(); it != nets.end(); it++) {

        if (it->second.left != -1 && it->second.right != -1) {
            for (int i = it->second.left; i <= it->second.right; ++i) {
                overlap[i]++;
            }
            it->second.valid = true;
        }
    }

    for (int i = 0; i < num_columns; i++) {
        density = std::max(density, overlap[i]);
    }
    in_.close();
    return std::move(system_ptr_);
}


}  // namespace routing

#endif  // SRC_ROUTING_INPUT_HPP_
