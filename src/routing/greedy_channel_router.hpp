#ifndef SRC_ROUTING_GREEDY_CHANNEL_ROUTER_HPP_
#define SRC_ROUTING_GREEDY_CHANNEL_ROUTER_HPP_


#include <routing/system.hpp>

namespace routing {

class GreedyChannelRouter {
 public:
    using system_ptr_type = std::shared_ptr<backend::System>;
    explicit GreedyChannelRouter(system_ptr_type system_ptr)
    : system_ptr_(system_ptr), num_columns_(system_ptr->num_columns) {}
    ~GreedyChannelRouter() = default;

    int route(int curr_col);
    system_ptr_type getdata();

 private:
    system_ptr_type system_ptr_;
    int num_columns_;
    int min_jog_ = 1;
    std::vector<std::vector<int>> raise_fall_table_;
    void initRaiseFallTable(int curr_col);

    bool isCompleteRoute();
    int calNumTracks();

    // 6 Greedy Heuristics
    void makeTopBottomConnection(int col, bool& is_top, bool& is_bottom);
    void collapseSplitNets(int col);
    void moveSplitNetCloserToOneAnother(int col);
    void raiseRisingNetsAndLowerFailingNet();
    void widenChannel(int col, bool is_top, bool is_bottom);
    void extendColumn(int col);


};

GreedyChannelRouter::system_ptr_type GreedyChannelRouter::getdata() {
    return std::move(system_ptr_);
}



// Check raise and fail table
// for (int pin = 0; pin < raise_fail_table_.size(); pin++) {
//     if (pin == 0) {
//         for (int col = 0; col < num_columns_; col++)
//             std::cout << system_ptr_->columns[col].top_pin << "\t";
//         std::cout << std::endl;
//     }
//     for (int col = 0; col < num_columns_; col++) {
//         std::cout << raise_fail_table_[pin][col] << "\t";
//     }
//     std::cout << std::endl;
//     if (pin == raise_fail_table_.size()-1) {
//         for (int col = 0; col < num_columns_; col++)
//             std::cout << system_ptr_->columns[col].bottom_pin << "\t";
//         std::cout << std::endl;
//     }
// }

// fgetc(stdin);

int GreedyChannelRouter::route(int curr_col) {
    auto& rows = system_ptr_->rows;
    // reset
    system_ptr_->all_vertical_branches.clear();
    system_ptr_->all_horizontal_trunks.clear();
    initRaiseFallTable(curr_col);

    // route complete
    for (int col = 0; col < num_columns_ || !isCompleteRoute(); ++col) {

        std::vector<int> temp_row_index(rows.size(), 0);
        for (int i = 0; i < rows.size() && col != 0; ++i)
            temp_row_index[i] = rows[i].next;

        if (col != 0) {
            rows.clear();
            rows.resize(temp_row_index.size());
        } else {
            // tracks >= max local density
            rows.resize(system_ptr_->density);
        }

        for (int i = 0; i < rows.size(); ++i) {
            if (col == 0)
                rows[i].horizontal = 0;
            else
                rows[i].horizontal = temp_row_index[i];
            rows[i].next = 0;
            rows[i].temp = 0;
            rows[i].vertical = 0;
        }

        /*------------- Steps A -----------------*/
        /// Make minimal feasible top and bottom connections
        bool is_top = false, is_bottom = false;
        makeTopBottomConnection(col, is_top, is_bottom);

        /*------------- Steps B -----------------*/
        /// to free up the most tracks
        collapseSplitNets(col);

        /*------------- Steps C -----------------*/
        moveSplitNetCloserToOneAnother(col);

        /*------------- Steps D -----------------*/
        raiseRisingNetsAndLowerFailingNet();

        /*------------- Steps E -----------------*/
        // widen channel when necessary
        widenChannel(col, is_top, is_bottom);

        /*------------- Steps F -----------------*/
        // may use a few column off the edge
        extendColumn(col);
    }

    return calNumTracks();
}

void GreedyChannelRouter::makeTopBottomConnection(int col, bool& is_top, bool& is_bottom) {
    const auto& top_pin = system_ptr_->columns[col].top_pin;
    const auto& bottom_pin = system_ptr_->columns[col].bottom_pin;
    auto& rows = system_ptr_->rows;

    // case: top and bot are 0
    if (top_pin == 0 && bottom_pin == 0) {
        is_top = true;
        is_bottom = true;
        return;
    }

    // case: there are other bottom pins
    if (bottom_pin != 0) {
        int iter = 0;  // column scan
        while (rows[iter].horizontal != 0 && rows[iter].horizontal != bottom_pin) {
            if (++iter < 0 || rows[iter].vertical != 0) {
                return;
            }
        }
        is_bottom = true;
        for (int i = 0; i <= iter; ++i) {
            rows[i].vertical = bottom_pin;
        }
        rows[iter].horizontal = bottom_pin;
    }

    // std::cout << " col is " << col << std::endl;
    // for (int i = 0; i < rows.size(); ++i) {
    //     std::cout <<" rows[" << i << "] = " << rows[i].horizontal << " " <<
    //     rows[i].vertical << std::endl;
    // }

    // fgetc(stdin);

    if (top_pin != 0) {
        int iter = rows.size() - 1;  // column scan
        while (rows[iter].horizontal != 0 && rows[iter].horizontal != top_pin) {
            if (--iter < 0 || rows[iter].vertical != 0) {
                return;
            }
        }
        for (int i = rows.size() - 1; i >= iter; --i) {
            rows[i].vertical = top_pin;
        }

        rows[iter].horizontal = top_pin;
        is_top = true;

        // std::cout << top_pin <<" " << bottom_pin << " "<< steps << std::endl;
    }
}


// to free up the most tracks
void GreedyChannelRouter::collapseSplitNets(int col) {
    const auto& top_pin = system_ptr_->columns[col].top_pin;
    const auto& bottom_pin = system_ptr_->columns[col].bottom_pin;
    auto& rows = system_ptr_->rows;

    for (int i = 0; i < rows.size(); ++i) {
        // rows[i].horizontal != 0 --> row.horizontal has been definded
        if (rows[i].horizontal != 0) {
            int collapse_net = rows[i].horizontal;
            int tmp_row = i + 1;
            while (tmp_row < rows.size() && rows[tmp_row].horizontal != collapse_net) {
                if (rows[tmp_row].vertical != 0 && rows[tmp_row].vertical != collapse_net)
                    tmp_row = rows.size();
                tmp_row += 1;
            }
            if (tmp_row < rows.size()) {
                for (int j = i; j <= tmp_row; j++) {
                    rows[j].vertical = collapse_net;
                }
                // raise
                if (raise_fall_table_[collapse_net][col] == 1) {
                    rows[i].temp = collapse_net;
                    rows[i].horizontal = 0;
                } else {
                    rows[tmp_row].temp = collapse_net;
                    rows[tmp_row].horizontal = 0;
                }
            }
        }
    }
}


void GreedyChannelRouter::moveSplitNetCloserToOneAnother(int col) {
    auto& rows = system_ptr_->rows;

    auto is_finish = [this] (int col, int sigment) -> bool {
        if (this->system_ptr_->nets[sigment].right >= col)
            return false;
        int num_tracks = 0;
        for (int i = 0; i < this->system_ptr_->rows.size(); ++i) {
            if (sigment == this->system_ptr_->rows[i].horizontal)
                num_tracks += 1;
        }
        if (num_tracks > 1)
            return false;
        return true;
    };

    for (int y1 = 0, y2 = rows.size() - 1; y1 <= y2; y1++, y2--) {
        if (y1 != y2) {
            if (rows[y1].horizontal != 0 && !is_finish(col, rows[y1].horizontal)) {
                if (raise_fall_table_[rows[y1].horizontal][col] == 1) {
                    int sigment = rows[y1].horizontal;
                    int tmp_row = y1;
                    while (tmp_row < rows.size()) {
                        if (rows[++tmp_row].vertical != 0)
                            break;
                    }
                    while (tmp_row > y1) {
                        if (rows[--tmp_row].horizontal == 0)
                            break;
                    }
                    if (tmp_row - y1 >= min_jog_) {
                        for (int j = y1; j <= tmp_row; j++) {
                            rows[j].vertical = sigment;
                        }
                        rows[tmp_row].horizontal = sigment;
                        rows[tmp_row].next = sigment;
                    } else {
                        rows[y1].next = sigment;
                    }
                } else if (raise_fall_table_[rows[y1].horizontal][col] == -1) {
                    int sig = rows[y1].horizontal;
                    int tmp_row = y1;
                    while (tmp_row >= 0) {
                        if (rows[--tmp_row].vertical != 0)
                            break;
                    }
                    while (tmp_row < y1) {
                        if (rows[++tmp_row].horizontal == 0)
                            break;
                    }
                    if (y1 - tmp_row >= min_jog_) {
                        for (int j = y1; j >= tmp_row; j--)
                            rows[j].vertical = sig;
                        rows[tmp_row].horizontal = sig;
                        rows[tmp_row].next = sig;
                    } else {
                        rows[y1].next = sig;
                    }
                } else {
                    // Change
                    rows[y1].next = rows[y1].horizontal;
                }
            }

            if (rows[y2].horizontal != 0 && !is_finish(col, rows[y2].horizontal)) {
                if (raise_fall_table_[rows[y2].horizontal][col] == 1) {
                    int sigment = rows[y2].horizontal;
                    int tmp_row = y2;
                    while (tmp_row < rows.size()) {
                        if (rows[++tmp_row].vertical != 0)
                            break;
                    }
                    while (tmp_row > y2) {
                        if (rows[--tmp_row].horizontal == 0)
                            break;
                    }
                    if (tmp_row - y2 >= min_jog_) {
                        for (int r = y2; r <= tmp_row; ++r) {
                            rows[r].vertical = sigment;
                        }
                        rows[tmp_row].horizontal = sigment;
                        rows[tmp_row].next = sigment;
                    } else {
                        rows[y2].next = sigment;
                    }
                } else if (raise_fall_table_[rows[y2].horizontal][col] == -1) {
                    int sigment = rows[y2].horizontal;
                    int tmp_row = y2;
                    while (tmp_row >= 0) {
                        if (rows[--tmp_row].vertical != 0)
                            break;
                    }
                    while (tmp_row < y2) {
                        if (rows[++tmp_row].horizontal == 0) {
                            break;
                        }
                    }
                    if (y2 - tmp_row >= min_jog_) {
                        for (int j = y2; j >= tmp_row; --j) {
                            rows[j].vertical = sigment;
                        }
                        rows[tmp_row].horizontal = sigment;
                        rows[tmp_row].next = sigment;
                    } else {
                        rows[y2].next = sigment;
                    }
                } else {
                    // Change
                    rows[y2].next = rows[y2].horizontal;
                }
            }
        } else {
            if (rows[y1].horizontal != 0 & !is_finish(col, rows[y1].horizontal)) {
                if (raise_fall_table_[rows[y1].horizontal][col] == 1) {
                    int sigment = rows[y1].horizontal;
                    int tmp_row = y1;
                    while (tmp_row < rows.size()) {
                        if (rows[++tmp_row].vertical != 0) {
                            break;
                        }
                    }
                    while (tmp_row > y1) {
                        if (rows[--tmp_row].horizontal == 0) {
                            break;
                        }
                    }
                    if (tmp_row - y1 >= min_jog_) {
                        for (int r = y1; r <= tmp_row; r++) {
                            rows[r].vertical = sigment;
                        }
                        rows[tmp_row].horizontal = sigment;
                        rows[tmp_row].next = sigment;
                    } else {
                        rows[y1].next = sigment;
                    }
                } else if (raise_fall_table_[rows[y1].horizontal][col] == -1) {
                    int sigment = rows[y1].horizontal;
                    int tmp_row = y1;
                    while (tmp_row >= 0) {
                        if (rows[--tmp_row].vertical != 0) {
                            break;
                        }
                    }
                    while (tmp_row < y1) {
                        if (rows[++tmp_row].horizontal == 0) {
                            break;
                        }
                    }
                    if (y1 - tmp_row >= min_jog_) {
                        for (int j = y1; j >= tmp_row; --j) {
                            rows[j].vertical = sigment;
                        }
                        rows[tmp_row].horizontal = sigment;
                        rows[tmp_row].next = sigment;
                    } else {
                        rows[y1].next = sigment;
                    }
                } else {
                    // Change
                    rows[y1].next = rows[y1].horizontal;
                }
            }
        }
    }
}


void GreedyChannelRouter::raiseRisingNetsAndLowerFailingNet() {
    auto&  rows = system_ptr_->rows;
    for (int i = 0; i < rows.size(); i++) {
        if (rows[i].temp != 0)
            rows[i].horizontal = rows[i].temp;
    }
}

void GreedyChannelRouter::widenChannel(int col, bool is_top, bool is_bottom) {
    const auto& top_pin = system_ptr_->columns[col].top_pin;
    const auto& bottom_pin = system_ptr_->columns[col].bottom_pin;
    auto& rows = system_ptr_->rows;
    auto& all_vertical_branches = system_ptr_->all_vertical_branches;
    auto& all_horizontal_trunks = system_ptr_->all_horizontal_trunks;

    if (!is_bottom && bottom_pin != 0) {
        int num_tmp_rows = 0;
        while (rows[num_tmp_rows].vertical == 0 && num_tmp_rows < rows.size() / 2)
            num_tmp_rows++;
        for (int i = 0; i < num_tmp_rows; i++) {
            rows[i].vertical = bottom_pin;
        }
        backend::Row temp_row{bottom_pin, bottom_pin, bottom_pin, bottom_pin};
        temp_row.temp = 0;
        rows.insert(rows.begin() + num_tmp_rows, temp_row);

        int y = num_tmp_rows + 1;
        // horizontal
        for (int i = 0; i < all_horizontal_trunks.size(); i++) {
            auto& tmp_hor = all_horizontal_trunks[i];
            tmp_hor.insert(tmp_hor.begin() + y, 0);
        }

        // vertical
        for (int i = 0; i < all_vertical_branches.size(); i++) {
            auto& tmp_ver = all_vertical_branches[i];
            if (y == tmp_ver.size())
                tmp_ver.insert(tmp_ver.begin() + y, tmp_ver[y - 1]);
            else if (y == 0)
                tmp_ver.insert(tmp_ver.begin() + y, tmp_ver[0]);
            else if (y > 0 && tmp_ver[y] == tmp_ver[y - 1])
                tmp_ver.insert(tmp_ver.begin() + y, tmp_ver[y]);
            else
                tmp_ver.insert(tmp_ver.begin() + y, 0);
        }
    }
    if (!is_top && top_pin != 0) {
        int num_tmp_rows = rows.size() - 1;
        while (rows[num_tmp_rows].vertical == 0 && num_tmp_rows > rows.size() / 2)
            num_tmp_rows--;
        for (int i = rows.size() - 1; i > num_tmp_rows; i--) {
            rows[i].vertical = top_pin;
        }
        backend::Row temp_row{top_pin, top_pin, top_pin, top_pin};
        temp_row.temp = 0;
        rows.insert(rows.begin() + num_tmp_rows + 1, temp_row);

        int y = num_tmp_rows + 1;

        // horizontal
        for (int i = 0; i < all_horizontal_trunks.size(); i++) {
            std::vector<int>& tmp_hor = all_horizontal_trunks[i];
            tmp_hor.insert(tmp_hor.begin() + y, 0);
        }

        // vertical
        for (int i = 0; i < all_vertical_branches.size(); i++) {
            std::vector<int>& tmp_ver = all_vertical_branches[i];
            if (y == tmp_ver.size())
                tmp_ver.insert(tmp_ver.begin() + y, tmp_ver[y - 1]);
            else if (y == 0)
                tmp_ver.insert(tmp_ver.begin() + y, tmp_ver[0]);
            else if (y > 0 && tmp_ver[y] == tmp_ver[y - 1])
                tmp_ver.insert(tmp_ver.begin() + y, tmp_ver[y]);
            else
                tmp_ver.insert(tmp_ver.begin() + y, 0);
        }
    }

    // fgetc(stdin);
}

void GreedyChannelRouter::extendColumn(int col) {
    auto& nets = system_ptr_->nets;
    auto& rows = system_ptr_->rows;
    auto& all_vertical_branches = system_ptr_->all_vertical_branches;
    auto& all_horizontal_trunks = system_ptr_->all_horizontal_trunks;
    for (int i = 0; i < rows.size(); i++) {
        bool condition1 = (rows[i].next == 0);
        bool condition2 = (nets[rows[i].next].right > col);
        int ntrack = 0;
        for (int j = 0; j < rows.size(); j++) {
            if (rows[i].next == rows[j].next)
                ntrack += 1;
        }
        bool condition3 = (ntrack > 1);
        if (!condition1 && !condition2 && !condition3)
            rows[i].next = 0;
    }
    // fgetc(stdin);

    std::vector<int> hor, ver;
    for (int i = 0; i < rows.size(); i++) {
        hor.push_back(rows[i].horizontal);
        ver.push_back(rows[i].vertical);
    }
    all_horizontal_trunks.push_back(hor);
    all_vertical_branches.push_back(ver);
}

bool GreedyChannelRouter::isCompleteRoute() {
    const auto& rows = system_ptr_->rows;
    for (int i = 0; i < rows.size(); i++)
        if (rows[i].next != 0)
            return false;
    return true;
}


int GreedyChannelRouter::calNumTracks() {
    auto& all_vertical_branches = system_ptr_->all_vertical_branches;
    const auto& all_horizontal_trunks = system_ptr_->all_horizontal_trunks;
    const auto& columns = system_ptr_->columns;
    for (int i = 0; i < all_vertical_branches.size(); ++i) {
        std::vector<int>& ver = all_vertical_branches[i];
        if (i < columns.size()) {
            if (columns[i].bottom_pin != 0)
                ver.insert(ver.begin(), columns[i].bottom_pin);
            else
                ver.insert(ver.begin(), 0);
            if (columns[i].top_pin != 0)
                ver.push_back(columns[i].top_pin);
            else
                ver.push_back(0);
        } else {
            ver.insert(ver.begin(), 0);
            ver.push_back(0);
        }
    }
    // modified
    return all_vertical_branches[0].size() - 2;
}

void GreedyChannelRouter::initRaiseFallTable(int curr_col) {
    const auto& columns = system_ptr_->columns;
    int max_pin_id = 0;
    for (int i = 0; i < num_columns_; i++) {
        max_pin_id = std::max(columns[i].top_pin, max_pin_id);
        max_pin_id = std::max(columns[i].bottom_pin, max_pin_id);
    }

    raise_fall_table_.clear();
    raise_fall_table_.resize(max_pin_id + 1);


    // raise_fall_table_
    // row : 0 ~ max_pin_id
    // col : num of  input colums

    for (int col = 0; col < raise_fall_table_.size(); col++) {
        raise_fall_table_[col].resize(num_columns_);
    }
    for (int col = 0; col < num_columns_; col++) {
        for (int pin = 0; pin < raise_fall_table_.size(); pin++) {
            bool fall = false, raise = false;
            for (int i = col + 1; i < num_columns_ && i <= col + curr_col; ++i) {
                if (columns[i].top_pin == pin) {
                    raise = true;
                }
                if (columns[i].bottom_pin == pin) {
                    fall = true;
                }
            }
            if (raise && fall)
                raise_fall_table_[pin][col] = 0;
            else if (raise)
                raise_fall_table_[pin][col] = 1;
            else if (fall)
                raise_fall_table_[pin][col] = -1;
            else
                raise_fall_table_[pin][col] = 0;

        }
    }
}







}  // namespace routing

#endif  // SRC_ROUTING_GREEDY_CHANNEL_ROUTER_HPP_
