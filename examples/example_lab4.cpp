// Copyright (c) 2022 Katelyn Bai
#include <routing/Lab4.hpp>
#include <chrono>
void post(char*, std::fstream&);

int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cout << "Usage: ./Lab4 <Input_flie> <Output_flie>" << std::endl;
    }
    std::ifstream in(argv[1], std::ifstream::in);
    std::fstream out(argv[2], std::fstream::out);

    std::chrono::high_resolution_clock::time_point start, end;
    start = std::chrono::high_resolution_clock::now();

    routing::Input input(in);
    auto data_ptr = input.readFile();
    // std::cout << "start lab4!!" <<" "<<data_ptr<< std::endl;

    // std::cout << "\n------Start Routing--------" << std::endl;
    int num_columns = data_ptr->num_columns;
    routing::GreedyChannelRouter greedyDetailRouter(std::move(data_ptr));

    // Try Different Initail-Channel-Width
    // Left-to-right, column-by-column scan
    int min_tracks = INT_MAX, select_col = 0;
    for (int curr_col = 0; curr_col < num_columns; curr_col++) {
        // steady-net-constant : windows size in term of # of columns
        // route
        int num_tracks = greedyDetailRouter.route(curr_col);

        // select col
        if (num_tracks < min_tracks) {
            min_tracks = num_tracks;
            select_col = curr_col;
        }
    }

    // route
    int num_tracks = greedyDetailRouter.route(select_col);
    auto data_ptr2 = greedyDetailRouter.getdata();
    // std::cout << "------End Routing--------" << std::endl;


    // output
    auto& nets = data_ptr2->nets;
    const auto& all_vertical_branches = data_ptr2->all_vertical_branches;
    const auto& all_horizontal_trunks = data_ptr2->all_horizontal_trunks;

    if (all_horizontal_trunks.size() <= data_ptr2->num_columns) {
        for (int i = 1; i < nets.size(); i++) {
            if (!nets[i].valid) {
                continue;
            } else {
                out << ".begin " << i << std::endl;
                for (int x = 0; x < all_vertical_branches.size(); x++) {
                    for (int y = 0; y < all_vertical_branches[0].size(); y++) {
                        if (all_vertical_branches[x][y] == i) {
                            out << ".V " << x << " " << y << " ";
                            while (y + 1 < all_vertical_branches[0].size() && all_vertical_branches[x][y + 1] == i)
                                y++;
                            out << y << std::endl;
                        }
                    }
                }
                for (int y = 0; y < all_horizontal_trunks[0].size(); y++) {
                    for (int x = 0; x < all_horizontal_trunks.size(); x++) {
                        if (all_horizontal_trunks[x][y] == i) {
                            if (x + 1 < all_horizontal_trunks.size() && all_horizontal_trunks[x + 1][y] == i) {
                                out << ".H " << x << " " << y + 1 << " ";
                                while (x + 1 < all_horizontal_trunks.size() && all_horizontal_trunks[x + 1][y] == i)
                                    x++;
                                out << x << std::endl;
                            }
                        }
                    }
                }
                out << ".end" << std::endl;
            }
        }
    } else {
        // postprocess for spill-over problem
        post(argv[1], out);
    }

    out.close();

    /*Timer*/
    // end = std::chrono::high_resolution_clock::now();
    // std::chrono::duration<double> duration = end - start;

    return 0;
}



void post(char* argv, std::fstream& out) {
    using routing::backend::Column;
    using routing::backend::net;
    std::map <int, net*> nets;
    std::map <int, Column*> columns;
    std::map <int, bool> occupied;  // track is occupied
    std::string s;
    std::ifstream input(argv, std::ifstream::in);
    int pin;
    int now_top_column_id = 0;
    int now_bottom_id = 0;
    int max_pin_id = 0;
    int num_tracks, half_track;
    int end_column = 0;

    std::getline(input, s);
    std::stringstream ss(s);
    while (ss >> pin) {
        std::map <int, net*>::iterator it;
        it = nets.find(pin);
        if (it == nets.end() && pin != 0) {
            net* temp_net = new net;
            nets.insert(std::pair<int, net*>(pin, temp_net));
            temp_net->top_column.push_back(now_top_column_id);
            temp_net->top_y = 0;
            temp_net->bottom_y = 0;
            temp_net->col_link_id = INT_MIN;
            temp_net->is_locked = 0;
            temp_net->top_right = INT_MIN;
            temp_net->top_left = INT_MAX;
            temp_net->bottom_right = INT_MIN;
            temp_net->bottom_left = INT_MAX;

            temp_net->is_top = false;
            temp_net->is_bottom = false;
            temp_net->is_selected = false;
        } else if (pin != 0) {
            it->second->top_column.push_back(now_top_column_id);
        }

        max_pin_id = std::max(max_pin_id, pin);
        now_top_column_id++;
    }
    std::getline(input, s);
    std::stringstream ss2(s);
    while (ss2 >> pin) {
        std::map <int, net*>::iterator it;
        it = nets.find(pin);
        if (it == nets.end() && pin != 0) {
            net* temp_net = new net;
            nets.insert(std::pair<int, net*>(pin, temp_net));
            temp_net->bottom_column.push_back(now_bottom_id);
            temp_net->top_y = 0;
            temp_net->bottom_y = 0;

            temp_net->col_link_id = INT_MIN;
            temp_net->is_locked = 0;
            temp_net->top_right = INT_MIN;
            temp_net->top_left = INT_MAX;
            temp_net->bottom_right = INT_MIN;
            temp_net->bottom_left = INT_MAX;

            // for check!!!!!
            temp_net->is_top = false;
            temp_net->is_bottom = false;
            temp_net->is_selected = false;
        } else if (pin != 0) {
            it->second->bottom_column.push_back(now_bottom_id);
        }
        Column* temp_column = new Column;
        temp_column->top_pin = now_top_column_id*2+1;
        temp_column->bottom_pin = 0;
        columns.insert(std::pair<int, Column*>(now_bottom_id, temp_column));
        now_bottom_id++;
    }
    half_track = now_top_column_id;

    // min num_tracks > local density
    num_tracks = 2*half_track;
    int num_nets = nets.size();


    // initialize occupied map
    for (int i = 0; i < num_tracks; ++i)
        occupied.insert(std::pair<int, bool>(i+1, false));

    int num_same_sides = 0;
    std::map<int, net*>::iterator it = nets.begin();
    for (; it != nets.end(); ++it) {
        if (it->second->top_column.size() == 0 || it->second->bottom_column.size() == 0) {
            it->second->is_same_side = true;
            num_same_sides++;
            if (it->second->bottom_column.size() == 0)
                it->second->is_top = true;
            else
                it->second->is_bottom = true;
        } else {
            it->second->is_same_side = false;
        }
    }

    // starting random
    std::srand(time(NULL));
    while (end_column < num_nets - num_same_sides) {
        end_column = 0;

        std::map<int, net*>::iterator it = nets.begin();
        for (; it != nets.end(); ++it) {
            it->second->col_link_id = INT_MIN;
            int rand_number;
            if (it->second->is_same_side == true) {
                bool hold = true;
                while (hold) {
                    if (it->second->is_top == true)
                        rand_number = (std::rand()%half_track) + half_track+1;
                    else
                        rand_number = (std::rand()%half_track) + 1;
                    if (occupied.find(rand_number)->second == false) {
                        hold = false;
                        if (it->second->is_top == 1)
                            it->second->top_y = rand_number;
                        else
                            it->second->bottom_y = rand_number;
                        occupied.find(rand_number)->second = true;  // update
                    }
                }
            } else {
                bool hold = true;
                while (hold) {
                    rand_number = (std::rand()%half_track)+half_track+1;
                    if (occupied.find(rand_number)->second == false) {
                        hold = false;
                        it->second->top_y = rand_number;
                        occupied.find(rand_number)->second = true;  // update
                    }
                }
                hold = true;
                while (hold) {
                    rand_number = (std::rand()%half_track)+1;
                    if (occupied.find(rand_number)->second == false) {
                        hold = false;
                        it->second->bottom_y = rand_number;
                        occupied.find(rand_number)->second = true;  // update
                    }
                }
            }
        }

        it = nets.begin();
        for (; it != nets.end(); ++it) {
            for (int i = 0; i < it->second->top_column.size(); ++i) {
                int col_id;
                col_id = it->second->top_column[i];
                columns[col_id]->top_pin = it->second->top_y;
            }
            for (int i =  0; i < it->second->bottom_column.size(); ++i) {
                int col_id;
                col_id = it->second->bottom_column[i];
                columns[col_id]->bottom_pin = it->second->bottom_y;
            }
        }
        for (std::map<int, Column*>::iterator itc = columns.begin();
            itc != columns.end(); ++itc) {
            for (std::map<int, net*>::iterator iter = nets.begin(); iter != nets.end(); ++iter)
                iter->second->is_selected = false;
            int top_bound = itc->second->top_pin;
            int bottom_bound = itc->second->bottom_pin;
            int col_id;
            int random_net;
            bool hold = true;
            bool end = false;
            int count = 0;
            col_id = itc->first;
            while (hold) {
                bool is_selected = false;
                while (!is_selected) {
                    random_net = std::rand()%max_pin_id+1;
                    if (nets.find(random_net) != nets.end()) {
                        if (nets[random_net]->is_selected == 0 &&
                            nets[random_net]->is_locked == 0 &&
                            nets[random_net]->is_same_side == 0) {
                            is_selected = true;
                            count++;
                        } else {
                            nets[random_net]->is_selected = true;
                            count++;
                        }
                        if (count == num_nets) {
                            is_selected = true;
                            end = true;
                        }
                    }
                }
                if (nets[random_net]->bottom_y >= bottom_bound
                && nets[random_net]->top_y <= top_bound
                && !end && nets[random_net]->is_same_side == 0) {
                    if (nets[random_net]->bottom_y == bottom_bound
                    && nets[random_net]->top_y == top_bound) {
                        hold = true;
                    } else {
                        nets[random_net]->is_locked = true;
                        nets[random_net]->col_link_id = col_id;
                        end_column++;
                        hold = false;
                    }
                }
                if (count >= num_nets)
                    hold = false;
            }
        }

        for (auto it = occupied.begin(); it != occupied.end(); ++it) {
            it->second = 0;
        }
        for (auto it = nets.begin(); it != nets.end(); ++it) {
            it->second->is_locked = false;
        }
    }
    for (auto it=nets.begin(); it != nets.end(); ++it) {
        for (int i = 0; i < it->second->top_column.size(); ++i) {
            if (it->second->top_column[i] > it->second->top_right)
                it->second->top_right = it->second->top_column[i];
            if (it->second->top_column[i] < it->second->top_left)
                it->second->top_left = it->second->top_column[i];
        }
        for (int i = 0; i < it->second->bottom_column.size(); ++i) {
            if (it->second->bottom_column[i] > it->second->bottom_right)
                it->second->bottom_right = it->second->bottom_column[i];

            if (it->second->bottom_column[i] < it->second->bottom_left)
                it->second->bottom_left = it->second->bottom_column[i];
        }
    }


    // Write data !!
    for (auto it = nets.begin(); it != nets.end(); ++it) {
        if (it->second->top_column.size() != 0
        && it->second->bottom_column.size() != 0) {
            out << ".begin " << it->first << "\n";
            for (int i = 0; i < it->second->top_column.size(); ++i) {
                out << ".V " << it->second->top_column[i]
                << " " << it->second->top_y << " " << num_tracks+1 << "\n";
            }
            for (int i = 0; i < it->second->bottom_column.size(); ++i) {
                out << ".V " << it->second->bottom_column[i] << " 0 " << it->second->bottom_y << "\n";
            }
            out << ".V " << it->second->col_link_id << " " << it->second->bottom_y << " " << it->second->top_y << "\n";
            if (it->second->col_link_id < it->second->top_left) {
                if (it->second->col_link_id < it->second->top_right)
                    out << ".H " << it->second->col_link_id << " "
                    << it->second->top_y << " " << it->second->top_right << "\n";
            } else if (it->second->col_link_id > it->second->top_right) {
                if (it->second->top_left < it->second->col_link_id)
                    out << ".H " << it->second->top_left << " "
                    << it->second->top_y << " " << it->second->col_link_id << "\n";
            } else if (it->second->top_column.size()!= 0) {
                if (it->second->top_left < it->second->top_right)
                    out << ".H " << it->second->top_left << " "
                    << it->second->top_y << " " << it->second->top_right << "\n";
            }
            if (it->second->col_link_id < it->second->bottom_left) {
                if (it->second->col_link_id < it->second->bottom_right)
                    out << ".H " << it->second->col_link_id << " "
                    << it->second->bottom_y << " " << it->second->bottom_right << "\n";
            } else if (it->second->col_link_id > it->second->bottom_right) {
                if (it->second->bottom_left < it->second->col_link_id)
                    out << ".H " << it->second->bottom_left << " "
                    << it->second->bottom_y << " " << it->second->col_link_id << "\n";
            } else if (it->second->bottom_column.size() != 0) {
                if (it->second->bottom_left < it->second->bottom_right)
                    out << ".H " << it->second->bottom_left << " "
                    << it->second->bottom_y << " " << it->second->bottom_right << "\n";
            }
        } else if (it->second->is_top == 1) {
            out << ".begin " << it->first << "\n";
            for (unsigned i = 0; i < it->second->top_column.size(); ++i) {
                out << ".V " << it->second->top_column[i] << " "
                << it->second->top_y << " " << num_tracks+1 << "\n";
            }
            if (it->second->top_left < it->second->top_right )
                out << ".H " << it->second->top_left << " "
                << it->second->top_y << " " << it->second->top_right << "\n";
        } else if (it->second->is_bottom == 1) {
            out << ".begin " << it->first << "\n";
            for (unsigned i = 0; i < it->second->bottom_column.size(); ++i) {
                out << ".V " << it->second->bottom_column[i]
            << " 0 " << it->second->bottom_y << "\n";
            }
            if (it->second->bottom_left < it->second->bottom_right)
                out << ".H " << it->second->bottom_left << " "
                << it->second->bottom_y << " " << it->second->bottom_right << "\n";
        }
        out<< ".end\n";
    }
}
