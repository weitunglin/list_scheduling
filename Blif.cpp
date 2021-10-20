#include "Blif.h"

std::string token(std::string s, std::string delim) {
  if (s.find(delim) == std::string::npos) {
    return s;
  }

  return s.substr(s.find(delim) + delim.length(), s.length());
}

std::vector<std::string> split(std::string s) {
  std::stringstream ss(s);
  std::string v;
  std::vector<std::string> res;

  while (ss >> v) {
    res.push_back(v);
  }

  return res;
}

int getLevel(Node* n, std::string name) {
  return getLevelRecursive(n, name, 0);
}

int getLevelRecursive(Node* n, std::string name, int l) {
  if (n == nullptr) {
    return 0;
  }

  if (n->m_name == name) {
    return l;
  }

  std::vector<int> downLevels;
  for (auto& i: n->m_next) {
    int downLevel = getLevelRecursive(i, name, l + 1);
    if (downLevel != 0) {
      downLevels.push_back(downLevel);
    }
  }

  if (downLevels.size()) {
    return *std::max_element(std::begin(downLevels), std::end(downLevels));
  }

  return 0;
}

Node::Node() : m_done(false), m_name(""), m_latency(0) {}

Node::Node(std::string name, bool done) : m_name(std::move(name)), m_done(done), m_latency(0) {}

void Node::setLatency(int l) {
  this->m_latency = l;
  this->m_done = true;
}

Blif::Blif(std::string filename, int andConstraint, int orConstraint, int notConstraint)
  : m_filename(filename), m_type(MLRCS) {
  this->parseFile();
  this->m_resource[AND] = andConstraint;
  this->m_resource[OR] = orConstraint;
  this->m_resource[NOT] = notConstraint;
}

Blif::Blif(std::string filename, int latencyConstraint)
  : m_filename(filename), m_type(MRLCS) {
  this->parseFile();
  this->m_resource[LATENCY] = latencyConstraint;
}

bool Blif::allDone() {
  std::queue<Node*> queue;
  for (auto& i: this->m_graph->m_next) {
    queue.push(i);
  } 

  while (!queue.empty()) {
    Node* node = queue.front();
    queue.pop();
    if (node->m_done == false) {
      return false;
    }
    
    for (auto& j: node->m_next) {
      queue.push(j);
    }
  }

  return true;
}

void Blif::ML_RCS() {
  if (this->m_type != MLRCS) {
    return;
  }
  
  std::vector<Node*> allNodes = this->filterNode([](Node* i){return true;});
  // std::cout << "all nodes ";
  // for (auto& i: allNodes) {
    // std::cout << i->m_name << " " << i << " ";
  // }
  // std::cout << std::endl;

  for (uint op = AND; op <= NOT; ++op) {
    std::vector<Node*> nodes = this->filterNode([=] (Node* n) { return n->m_op == op; });
    if (nodes.size() > 0 && this->m_resource[op] < 1) {
      this->m_feasible = false;
      return;
    }
  }

  std::function<bool(Node*)> notDone = [] (Node* i) { return i->m_done == false; };

  int l = 0;
  while (!this->allDone()) {
    ++l;

    for (uint op = AND; op <= NOT; ++op) {
      // std::cout << "i" << op << "\n";
      // std::cout << "l" << l << "\n";

      std::function<bool(Node*)> isReady = [=] (Node* i) { return i->m_done == false && i->m_op == op && i->m_prev.size() > 0 && std::all_of(std::begin(i->m_prev), std::end(i->m_prev), [] (Node* j) { return j->m_done == true; }); };
      std::vector<Node*> prevDone = this->filterNode(isReady);
      // std::cout << "prev done";
      // if (prevDone.size() > 0) {
        // std::sort(std::begin(prevDone), std::end(prevDone), [] (Node* a, Node* b) { return std::move(a->m_name) >= std::move(b->m_name); });
      // }
      // for (auto& n: prevDone) {
      //   std::cout << n->m_name << " ";
      // }
      // std::cout << "\n";
      std::vector<Node*> readyNodes;
      for (auto& n: prevDone) {
        if (std::all_of(std::begin(n->m_prev), std::end(n->m_prev), [=] (Node* j) { return j->m_latency < l; })) {
          readyNodes.push_back(n);
        }
      }
      // std::cout << "ready nodes";
      // for (auto& n: readyNodes) {
      //   std::cout << n->m_name << " ";
      // }
      // std::cout << "\n";
      if (readyNodes.size() > this->m_resource[op]) {
        readyNodes.resize(this->m_resource[op]);
      }

      for (auto& j: readyNodes) {
        j->setLatency(l);
        // std::cout << "set" << j->m_name << j->m_latency << std::endl;
      }
      // std::cout << "l" << l << "\n\n";
    }
  }

  this->m_resource[LATENCY] = l;
  for (uint op = AND; op <= NOT; ++op) {
    int m = INT_MIN;
    for (int i = 1; i < this->m_resource[LATENCY]; ++i) {
      std::vector<Node*> nodes = this->filterNode([=] (Node* n) { return n->m_latency == i && n->m_op == op; });
      m = std::max(m, static_cast<int>(nodes.size()));
    }

    this->m_resource[op] = m;
  }
}

void Blif::MR_LCS() {
  if (this->m_type != MRLCS) {
    return;
  }

  // std::cout << "all nodes ";
  // for (auto& i: this->m_allNodes) {
  //   std::cout << i->m_name << " " << i->m_latency << " ";
  // }
  // std::cout << "\n\n";

  int max_latency = this->m_resource[LATENCY];
  // start ALAP 
  std::function<bool(Node*)> lastLayerNotDone = [] (Node* n) { return n->m_next.size() == 0 && n->m_done == false; };
  std::vector<Node*> lastLayer;
  while ((lastLayer = this->filterNode(lastLayerNotDone)).size() > 0) {
    int t = max_latency;
    Node* i = lastLayer.front();
    i->setLatency(t);
    /*
    while (true) {
      std::cout << "t " << t << std::endl;
      int alreadyScheduled = this->filterNode([=] (Node* n) {
        return n->m_done && n->m_op == i->m_op && n->m_latency == t;
      }).size();

      if (alreadyScheduled < m_resource[i->m_op]) {
        i->setLatency(t);
        break;
      }
      --t;
    }
    */
  }

  for (uint op = AND; op <= NOT; ++op) {
    this->m_resource[op] = 1;
  }

  std::function<bool(Node*)> notDone = [] (Node* n) { return n->m_done == false; };
  std::function<bool(Node*)> isReady = [] (Node* n) { return n->m_level != 1 && n->m_done == false && n->m_next.size() > 0 && std::all_of(std::begin(n->m_next), std::end(n->m_next), [] (Node* m) { return m->m_done == true; }); };

  while (!this->allDone()) {
    for (uint op = AND; op <= NOT; ++op) {
      std::vector<Node*> readyNodes = this->filterNode([=] (Node* n) { return isReady(n) && n->m_op == op; });
      // std::cout << "ready nodes ";
      // for (auto& n: readyNodes) {
      //   std::cout << n->m_name << " ";
      // }
      // std::cout << "\n";
      // if (!readyNodes.size()) {
        // std::vector<Node*> notDoneNodes = this->filterNode(notDone);

        // std::cout << "not done nodes ";
        // for (auto& m: notDoneNodes) {
        //   std::cout << m->m_name << " ";
        // }
        // std::cout << "\n";
      // }

      for (auto& i: readyNodes) {
        // std::cout << "node " << i->m_name;
        int t = this->m_resource[LATENCY];
        for (auto& j: i->m_next) {
          // std::cout << ", min " << j->m_latency << ":" << j->m_op; 
          t = std::min(t, (j->m_latency) - (1));
        }

        i->setLatency(t);
        // std::cout << "set " << i->m_name << i->m_latency << std::endl;
        // int alreadyScheduled = this->filterNode([=] (Node* n) {
        //   return n->m_done && n->m_op == op && n->m_latency == t;
        // }).size();
 
        // if (alreadyScheduled < m_resource[op]) {
        //   i->setLatency(t);
        //   std::cout << "set " << i->m_name << i->m_latency << std::endl;
        // } else {
        //   i->setLatency(t - 1);
        //   std::cout << "set - " << i->m_name << i->m_latency << std::endl;
        // }
      }
    }
  }
  // end ALAP

  // check feasible
  if (this->filterNode([] (Node* n) { return n->m_level > 1 && n->m_latency < 1; }).size()) {
    this->m_feasible = false;
    return;
  }

  for (auto& i: this->filterNode([] (Node* n) { return true; })) {
    // std::cout << i->m_name << " " << i->m_done << " " << i->m_latency << " " << i->m_level << std::endl;
    i->m_done = false;
  }
  for (auto& i: this->m_inputs) {
    this->filterNode([=] (Node* n) { return n->m_name == i; })[0]->m_done = true;
  }

  int latency = 1;
  do {
    for (uint op = AND; op <= NOT; ++op) {
      std::function<bool(Node*)> done = [=] (Node* n) { return n->m_done == false && n->m_op == op && n->m_prev.size() > 0 && std::all_of(std::begin(n->m_prev), std::end(n->m_prev), [] (Node* m) { return m->m_done == true; }); };
      std::vector<Node*> prevDone = this->filterNode(done);

      std::vector<Node*> readyNodes;
      for (auto& i: prevDone) {
        if (std::all_of(std::begin(i->m_prev), std::end(i->m_prev), [=] (Node* j) { return j->m_latency < latency; })) {
          readyNodes.push_back(i);
        } else {
          // std::cout << i->m_name << " more latency " << latency << " < " << std::endl;
          // for (auto& j: i->m_prev) {
          //   std::cout << j->m_name << ":" << j->m_latency << ", ";
          // }
          // std::cout << std::endl;
        }
      }

      // std::cout << "ready nodes ";
      // for (auto& n: readyNodes) {
      //   std::cout << n->m_name << " ";
      // }
      // std::cout << "\n";
      if (!readyNodes.size()) {
        std::vector<Node*> notDoneNodes = this->filterNode(notDone);

        // std::cout << "not done nodes ";
        // for (auto& m: notDoneNodes) {
        //   std::cout << m->m_name << " ";
        // }
        // std::cout << "\n";
      }
      std::sort(std::begin(readyNodes), std::end(readyNodes), [=] (Node* a, Node* b) { return (a->m_latency - latency) < (b->m_latency - latency); });

      // count how many nodes need to be process immediatly
      int immediate = 0;
      for (auto& i: readyNodes) {
        int slack = i->m_latency - latency;
        // std::cout << i->m_name << " " << slack << std::endl;
        if (slack == 0) {
          ++immediate;
        }
      }

      // update resource constraint
      if (immediate > this->m_resource[op]) {
        this->m_resource[op] = immediate;
      }

      if (readyNodes.size() > this->m_resource[op]) {
        readyNodes.resize(this->m_resource[op]);
      }

      for (auto& i: readyNodes) {
        i->setLatency(latency);
        // std::cout << "set " << i->m_name << i->m_latency << std::endl;
      }
    }

    ++latency;
    // std::cout << "latency " << latency << std::endl;
  } while (this->filterNode(notDone).size() != 0);
  this->m_resource[LATENCY] = latency - 1;
}

std::vector<Node*> Blif::filterNode(std::function<bool(Node*)> f) {
  if (this->m_allNodes.size() == 0) {
    std::queue<Node*> queue;
    for (auto& i: this->m_graph->m_next) {
      queue.push(i);
    } 

    while (!queue.empty()) {
      Node* node = queue.front();
      queue.pop();
      this->m_allNodes.push_back(node);
    
      for (auto& j: node->m_next) {
        queue.push(j);
      }
    }
  }

  std::vector<Node*> result;
  for (auto& i: this->m_allNodes) {
    if (f(i)) {
      result.push_back(i);
    }
  }

  std::sort(std::begin(result), std::end(result), [] (Node* a, Node* b) { return a->m_name < b->m_name; });
  result.erase(std::unique(std::begin(result), std::end(result)), std::end(result));

  if (this->m_type == MLRCS) {
    std::sort(std::begin(result), std::end(result), [] (Node* a, Node* b) {
      return a->m_next.size() == b->m_next.size()
        ? std::max_element(std::begin(a->m_next), std::end(a->m_next), [] (Node const * u, Node const* v) { return u->m_level > v->m_level; }) > std::max_element(std::begin(b->m_next), std::end(b->m_next), [] (Node const * u, Node const* v) { return u->m_level > v->m_level; })
        : a->m_next.size() > b->m_next.size();
    });
  } else if (this->m_type == MRLCS) {
    std::sort(std::begin(result), std::end(result), [] (Node* a, Node* b) {
      return a->m_prev.size() == b->m_prev.size()
        ? std::max_element(std::begin(a->m_prev), std::end(a->m_prev), [] (Node const * u, Node const* v) { return u->m_level < v->m_level; }) > std::max_element(std::begin(b->m_prev), std::end(b->m_prev), [] (Node const * u, Node const* v) { return u->m_level < v->m_level; })
        : a->m_prev.size() > b->m_prev.size();
    });
  }
  return result;
}

void Blif::parseFile() {
  std::ifstream f(m_filename);
  std::string line;
  if (!f.is_open()) {
    std::cerr << "Can not open file correctly\n";
    return;
  }

  do {
    getline(f, line);
  } while (line.find(".model") == std::string::npos);

  std::string model = token(line, ".model");
  this->m_model = model;

  // inputs
  bool cont = false;
  do {
    getline(f, line);
    std::string inputs = token(line, ".inputs ");
    std::vector<std::string> tokened = split(inputs);
    if (tokened.back() == "\\") {
      tokened.pop_back();
      cont = true;
    } else {
      cont = false;
    }

    this->m_inputs.insert(this->m_inputs.end(), std::make_move_iterator(std::begin(tokened)), std::make_move_iterator(std::end(tokened)));
  } while (cont);

  this->m_graph = new Node;
  for (const auto& i: this->m_inputs) {
    Node* newNode = new Node(i, true);
    this->m_graph->m_next.push_back(newNode);
    this->m_table.insert({ i, newNode });
  }

  // outputs
  cont = false;
  do {
    getline(f, line);
    std::string inputs = token(line, ".outputs ");
    std::vector<std::string> tokened = split(inputs);
    if (tokened.back() == "\\") {
      tokened.pop_back();
      cont = true;
    } else {
      cont = false;
    }

    this->m_outputs.insert(this->m_outputs.end(), std::make_move_iterator(std::begin(tokened)), std::make_move_iterator(std::end(tokened)));
  } while (cont);

  while (getline(f, line)) {
    if (line.find(".end") != std::string::npos) {
      break;
    }

    if (line.find(".names") != std::string::npos) {
      std::vector<std::string> names = split(token(line, ".names "));
      std::vector<std::string> functions;
 
      while (f.peek() != '.') {
        std::string l;
        getline(f, l);
        functions.push_back(l);
      }

      // identify operation type
      // +, *, !
      uint op;
      if (functions.size() == 1) {
        if (functions[0].front() == '0' && functions[0].back() == '1') {
          op = NOT;
        } else {
          op = AND;
        }
      } else {
        op = OR;
      }

      Node* result = nullptr;
      if (!this->m_table.count(names.back())) {
        result = new Node(names.back(), false);
        result->m_op = op;
        this->m_table.insert({ names.back(), result });
      } else {
        result = this->m_table[names.back()];
      }

      for (size_t i = 0; i < names.size() - 1; ++i) {
        Node* prev = nullptr;
        if (this->m_table.count(names[i])) {
          prev = this->m_table[names[i]];
          prev->m_next.push_back(result);
          result->m_prev.push_back(prev);
        } else {
          prev = new Node(names[i], false);
          this->m_table.insert({ names[i], prev });
          prev->m_next.push_back(result);
          result->m_prev.push_back(prev);
        }
      }
    }
  }

  f.close();

  
  std::vector<Node*> allNodes = this->filterNode([] (Node* n) { return true; });
  this->m_allNodes = allNodes;
  for (auto& i: allNodes) {
    i->m_level = getLevel(this->m_graph, i->m_name);
    // std::cout << i->m_name << " " << i->m_level << " " << i->m_op << std::endl;
  }
}

std::ostream& operator<<(std::ostream& os, Blif& rhs) {
  if (rhs.m_type == MLRCS) {
    os << "Resource-constrained Scheduling\n";
  } else if (rhs.m_type == MRLCS) {
    os << "Latency-constrained Scheduling\n";
  } else {
    return os;
  }

  if (!rhs.m_feasible) {
    os << "No Feasible solution\n";
  } else {
    for (int l = 1; l <= rhs.m_resource[LATENCY]; ++l) {
      os << l << ":";
      for (uint op = AND; op <= NOT; ++op) {
        os << " {";
        std::vector<Node*> nodes = rhs.filterNode([=] (Node* n) { return n->m_latency == l && n->m_op == op; });
        if (nodes.size()) {
          os << std::accumulate(std::next(std::begin(nodes)), std::end(nodes), std::move(nodes.front()->m_name), [] (std::string s, Node* n) {
            return s + " " + n->m_name;
          });
        }
        os << "}";
      }

      os << "\n";
    }

    for (uint i = AND; i <= NOT; ++i) {
      os << "#" << rhs.m_resource_name[i] << ": " << rhs.m_resource[i] << "\n";
    }
  }

  os << "END\n";
  return os;
}
