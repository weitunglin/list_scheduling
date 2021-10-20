#ifndef BLIF_H
#define BLIF_H
#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <map>
#include <string>
#include <sstream>
#include <iterator>
#include <numeric>
#include <algorithm>

class Node;

// type
const uint MLRCS = 0;
const uint MRLCS = 1;

// operator
const uint AND = 0;
const uint OR = 1;
const uint NOT = 2;
const uint LATENCY = 3;

// Utils Functions
std::string token(std::string s, std::string delim);
std::vector<std::string> split(std::string s);
int getLevel(Node* n, std::string name);
int getLevelRecursive(Node* n, std::string name, int l);


class Node {
public:
  Node();
  Node(std::string name, bool done);
  void setLatency(int l);

  bool m_done;
  std::string m_name;
  uint m_op = 0;
  int m_latency = 0;
  int m_level;
  std::vector<Node*> m_next;
  std::vector<Node*> m_prev;
};

class Blif : public Node {
public:
  Blif(std::string filename, int andConstraint, int orConstraint, int notConstraint);
  Blif(std::string filename, int latencyConstraint);
  
  void ML_RCS();
  void MR_LCS();  

  friend std::ostream& operator<<(std::ostream& os, Blif& rhs);

private:
  void parseFile();
  std::vector<Node*> filterNode(std::function<bool(Node*)> f);
  bool allDone();

  std::string m_filename;
  uint m_type;
  std::string m_model;
  std::vector<std::string> m_inputs;
  std::vector<std::string> m_outputs;
  Node* m_graph;
  bool m_feasible = true;
  std::map<std::string, Node*> m_table;
  std::vector<Node*> m_allNodes;

  int m_resource[4];
  std::string m_resource_name[3] = { "AND", "OR", "NOT" };
  int m_resource_delay[3] = { 1, 1, 1 };
};
#endif

