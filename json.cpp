#include "json.h"

using namespace std;


namespace Json {

    Document::Document(Node root) : root(move(root)) {
    }

    const Node &Document::GetRoot() const {
        return root;
    }

    Node LoadNode(istream &input);

    Node LoadArray(istream &input) {
        vector<Node> result;

        for (char c; input >> c && c != ']';) {
            if (c != ',') {
                input.putback(c);
            }
            result.push_back(LoadNode(input));
        }

        return Node(move(result));
    }

    Node LoadInt(istream &input) {
        int result = 0;


        while (isdigit(input.peek())) {
            result *= 10;
            result += input.get() - '0';
        }
        return Node(result);
    }

    Node LoadNumber(istream &input) {
        string number;
        int mantissa = 0;
        bool PassedPoint = false;

        bool IsNegative = input.peek() == '-' ? true : false;
        if (IsNegative)
            input.ignore();

        while (isdigit(input.peek()) || input.peek() == '.') {
            if (input.peek() != '.') {
                number += input.get();
                if (!PassedPoint)
                    ++mantissa;
            } else {
                input.ignore();
                PassedPoint = true;
            }
        }

        if (mantissa == number.size())
            return IsNegative
                   ? Node(-stoi(number))
                   : Node(stoi(number));

        // 56.873; ms = 3;
        string number_with_floating_point =
                string{number.begin(), number.begin() + mantissa} +
                "." +
                string{number.begin() + mantissa, number.end()};
        return IsNegative
               ? Node(-stod(number_with_floating_point))
               : Node(stod(number_with_floating_point));
    }

    Node LoadString(istream &input) {
        string line;
        getline(input, line, '"');
        return Node(move(line));
    }

    Node LoadDict(istream &input) {
        map<string, Node> result;

        for (char c; input >> c && c != '}';) {
            if (c == ',') {
                input >> c;
            }

            string key = LoadString(input).AsString();
            input >> c;
            result.emplace(move(key), LoadNode(input));
        }

        return Node(move(result));
    }
    Node LoadBool(istream &input) {
        if (input.peek() == 't') {
            input.ignore(4);
            return Node(true);
        } else {
            input.ignore(5);
            return Node(false);
        }
    }

    Node LoadNode(istream &input) {
        char c;
        input >> c;

        if (c == '[') {
            return LoadArray(input);
        } else if (c == '{') {
            return LoadDict(input);
        } else if (c == '"') {
            return LoadString(input);
        } else if (c == 't' || c == 'f') {
            input.putback(c);
            return LoadBool(input);
        } else {
            input.putback(c);
            return LoadNumber(input);
        }
    }
    Document Load(istream &input) {
        return Document{LoadNode(input)};
    }

}// namespace Json
