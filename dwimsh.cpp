#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <algorithm>

using namespace std;

// Comandos internos del shell
const vector<string> internalCommands = {"exit"};

// Función para calcular la distancia de Levenshtein
int levenshteinDistance(const string& s1, const string& s2) {
    int len1 = s1.size(), len2 = s2.size();
    vector<vector<int>> dp(len1 + 1, vector<int>(len2 + 1));

    for (int i = 0; i <= len1; i++) dp[i][0] = i;
    for (int j = 0; j <= len2; j++) dp[0][j] = j;

    for (int i = 1; i <= len1; i++) {
        for (int j = 1; j <= len2; j++) {
            if (s1[i - 1] == s2[j - 1]) {
                dp[i][j] = dp[i - 1][j - 1];
            } else {
                dp[i][j] = min({dp[i - 1][j] + 1, dp[i][j - 1] + 1, dp[i - 1][j - 1] + 1});
            }
        }
    }
    return dp[len1][len2];
}

// Función para dividir la línea de comandos en tokens
vector<string> tokenize(const string& line) {
    vector<string> tokens;
    stringstream ss(line);
    string token;
    while (ss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

// Función para obtener una lista de comandos en /usr/bin
vector<string> getCommands() {
    vector<string> commands;
    string dir = "/usr/bin";

    DIR* d = opendir(dir.c_str());
    if (!d) return commands;

    struct dirent* entry;
    while ((entry = readdir(d)) != nullptr) {
        if (entry->d_type == DT_REG || entry->d_type == DT_LNK || entry->d_type == DT_UNKNOWN) {
            commands.push_back(entry->d_name);
        }
    }
    closedir(d);
    return commands;
}

// Función para encontrar los comandos más similares ordenados por similitud
vector<string> findClosestCommands(const string& cmd, const vector<string>& commands) {
    vector<pair<int, string>> candidates;

    for (const string& candidate : commands) {
        int dist = levenshteinDistance(cmd, candidate);
        if (dist <= max(2, (int)(cmd.size() * 0.4))) { 
            candidates.emplace_back(dist, candidate);
        }
    }

    // Ordenar por menor distancia
    sort(candidates.begin(), candidates.end());

    // Extraer solo los nombres de los comandos
    vector<string> sortedCommands;
    for (const auto& pair : candidates) {
        sortedCommands.push_back(pair.second);
    }

    return sortedCommands;
}

// Función para ejecutar un comando
void executeCommand(vector<string>& args) {
    vector<char*> argv;
    for (auto& arg : args) {
        argv.push_back(&arg[0]);
    }
    argv.push_back(nullptr);

    pid_t pid = fork();
    if (pid == 0) {
        execvp(argv[0], argv.data());
        perror("execvp");
        exit(1);
    } else {
        waitpid(pid, nullptr, 0);
    }
}

int main() {
    string line;
    cout << "Bienvenido a dwimsh! - Escrito por Oliver Iraheta" << endl;
    vector<string> commands = getCommands();

    while (true) {
        cout << "\ndwimsh> ";
        if (!getline(cin, line)) {
            cout << "\nSaliendo de dwimsh..." << endl;
            break;
        }

        vector<string> args = tokenize(line);
        if (args.empty()) continue;

        // Manejo de comando interno "exit"
        if (args[0] == "exit") break;

        // Si el comando es válido, ejecutarlo directamente
        if (find(commands.begin(), commands.end(), args[0]) != commands.end()) {
            executeCommand(args);
        } else {
            // Obtener una lista de sugerencias ordenadas por similitud
            vector<string> suggestions = findClosestCommands(args[0], commands);

            for (const string& suggestion : suggestions) {
                cout << "Quiere decir '" << suggestion;
                for (size_t i = 1; i < args.size(); ++i) {
                    cout << " " << args[i];
                }
                cout << "'? [s/n]: ";

                char response;
                cin >> response;
                cin.ignore();

                if (response == 's' || response == 'S') {
                    args[0] = suggestion;
                    executeCommand(args);
                    break;
                }
            }

            if (suggestions.empty()) {
                cout << "No entiendo qué quiere hacer, pruebe de nuevo." << endl;
            }
        }
    }

    return 0;
}
