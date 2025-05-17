#include <wx/wx.h>
#include <wx/dcbuffer.h>
#include <vector>
#include <thread>
#include <chrono>
#include <algorithm>
#include <random>
#include <unordered_set>

using namespace std;

class SortingPanel : public wxPanel {
public:
    SortingPanel(wxFrame* parent)
        : wxPanel(parent), sorting(false), speed(50) {
        array.resize(20);
        
        unordered_set<int> uniqueNumbers;
        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<> dis(1, 100);

        while (uniqueNumbers.size() < array.size()) {
            uniqueNumbers.insert(dis(gen));
        }

        array.assign(uniqueNumbers.begin(), uniqueNumbers.end());
        shuffle(array.begin(), array.end(), gen);

        SetBackgroundStyle(wxBG_STYLE_PAINT);
        Bind(wxEVT_PAINT, &SortingPanel::OnPaint, this);
    }

    void StartSorting(void (*sortFunc)(vector<int>&, SortingPanel*)) {
        sorting = true;
        currentIndex1 = -1; 
        currentIndex2 = -1;
        thread([this, sortFunc]() {
            sortFunc(array, this);
            sorting = false;
            }).detach();
    }

    void UpdateVisualization(int index1 = -1, int index2 = -1) {
        currentIndex1 = index1;
        currentIndex2 = index2;
        Refresh();
        Update();
        
        int delay = (101 - speed) * 5; 
        this_thread::sleep_for(chrono::milliseconds(delay));
    }


    void OnPaint(wxPaintEvent&) {
        wxBufferedPaintDC dc(this);
        dc.Clear();

        if (array.empty()) return;

        int barWidth = GetSize().GetWidth() / array.size();
        int maxValue = *max_element(array.begin(), array.end());
        int height = GetSize().GetHeight();

        for (size_t i = 0; i < array.size(); ++i) {
            int barHeight = static_cast<int>(height * array[i] / static_cast<double>(maxValue) * 0.75);
            
            wxBrush brush(*wxBLUE_BRUSH);
            if (i == currentIndex1 || i == currentIndex2) {
                brush = *wxRED_BRUSH;
            }
            
            dc.SetBrush(brush);
            dc.SetPen(*wxTRANSPARENT_PEN);
            dc.DrawRectangle(i * barWidth, height - barHeight, barWidth - 2, barHeight);
            
            dc.SetTextForeground(*wxBLACK);
            dc.DrawText(to_string(array[i]),
                i * barWidth + barWidth / 4, height - barHeight - 20);
        }
    }

    void AddBar(int value) {
        if (array.size() < 100) { 
            if (find(array.begin(), array.end(), value) == array.end()) { 
                array.push_back(value);
                Refresh();
            }
        }
    }

    void RefreshArray() {
        array.clear(); 

        unordered_set<int> uniqueNumbers;
        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<> dis(1, 100);

        while (uniqueNumbers.size() < 20) { 
            uniqueNumbers.insert(dis(gen));
        }

        array.assign(uniqueNumbers.begin(), uniqueNumbers.end()); 
        shuffle(array.begin(), array.end(), gen); 

        Refresh(); 
    }

    void SetSpeed(int newSpeed) {
        speed = newSpeed;
    }

    vector<int> array;
    bool sorting;
    int speed; 
    int currentIndex1; 
    int currentIndex2; 
};


static void BubbleSort(vector<int>& array, SortingPanel* panel) {
    for (size_t i = 0; i < array.size() - 1; ++i) {
        for (size_t j = 0; j < array.size() - i - 1; ++j) {
            panel->UpdateVisualization(j, j + 1);
            if (array[j] > array[j + 1]) {
                swap(array[j], array[j + 1]);
                panel->UpdateVisualization(j, j + 1); 
            }
        }
    }
}

static void InsertionSort(vector<int>& array, SortingPanel* panel) {
    for (size_t i = 1; i < array.size(); ++i) {
        int key = array[i];
        int j = i - 1;
        panel->UpdateVisualization(j, i); 
        while (j >= 0 && array[j] > key) {
            array[j + 1] = array[j];
            --j;
            panel->UpdateVisualization(j + 1, i); 
        }
        array[j + 1] = key;
        panel->UpdateVisualization(j + 1, i); 
    }
}

static void SelectionSort(vector<int>& array, SortingPanel* panel) {
    for (size_t i = 0; i < array.size() - 1; ++i) {
        size_t minIndex = i;
        for (size_t j = i + 1; j < array.size(); ++j) {
            panel->UpdateVisualization(j, minIndex);
            if (array[j] < array[minIndex]) {
                minIndex = j;
            }
        }
        if (minIndex != i) {
            swap(array[i], array[minIndex]);
            panel->UpdateVisualization(i, minIndex); 
        }
    }
}

static void Merge(vector<int>& array, int left, int mid, int right, SortingPanel* panel) {
    int n1 = mid - left + 1;
    int n2 = right - mid;

    vector<int> L(n1), R(n2);
    for (int i = 0; i < n1; ++i)
        L[i] = array[left + i];
    for (int j = 0; j < n2; ++j)
        R[j] = array[mid + 1 + j];

    int i = 0, j = 0, k = left;

    while (i < n1 && j < n2) {
        panel->UpdateVisualization(k, (i < n1 ? left + i : right + j)); 
        if (L[i] <= R[j]) {
            array[k++] = L[i++];
        }
        else {
            array[k++] = R[j++];
        }
    }

    while (i < n1) {
        array[k++] = L[i++];
    }
    while (j < n2) {
        array[k++] = R[j++];
    }

    panel->UpdateVisualization(); 
}

static void MergeSortHelper(vector<int>& array, int left, int right, SortingPanel* panel) {
    if (left < right) {
        int mid = left + (right - left) / 2;
        MergeSortHelper(array, left, mid, panel);
        MergeSortHelper(array, mid + 1, right, panel);
        Merge(array, left, mid, right, panel);
    }
}

static void MergeSort(vector<int>& array, SortingPanel* panel) {
    MergeSortHelper(array, 0, array.size() - 1, panel);
}

static int Partition(vector<int>& array, int low, int high, SortingPanel* panel) {
    int pivot = array[high];
    int i = low - 1;

    for (int j = low; j < high; ++j) {
        panel->UpdateVisualization(j, high); 
        if (array[j] < pivot) {
            swap(array[++i], array[j]);
            panel->UpdateVisualization(i, j);
        }
    }
    swap(array[i + 1], array[high]);
    panel->UpdateVisualization(i + 1, high); 
    return i + 1;
}

static void QuickSortHelper(vector<int>& array, int low, int high, SortingPanel* panel) {
    if (low < high) {
        int pi = Partition(array, low, high, panel);
        QuickSortHelper(array, low, pi - 1, panel);
        QuickSortHelper(array, pi + 1, high, panel);
    }
}

static void QuickSort(vector<int>& array, SortingPanel* panel) {
    QuickSortHelper(array, 0, array.size() - 1, panel);
}

class SortingVisualizerApp : public wxApp {
public:
    bool OnInit() override {
        wxFrame* frame = new wxFrame(NULL, wxID_ANY, "Sorting Visualizer", wxDefaultPosition, wxSize(1280, 800));
        SortingPanel* panel = new SortingPanel(frame);

        wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
        wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);

        wxTextCtrl* numberInput = new wxTextCtrl(frame, wxID_ANY, "", wxDefaultPosition, wxSize(100, -1));
        wxButton* addButton = new wxButton(frame, wxID_ANY, "Add Bar");
        wxSlider* speedSlider = new wxSlider(frame, wxID_ANY, 50, 1, 100, wxDefaultPosition, wxSize(200, -1));


        wxButton* bubbleSortButton = new wxButton(frame, wxID_ANY, "Bubble Sort");
        wxButton* insertionSortButton = new wxButton(frame, wxID_ANY, "Insertion Sort");
        wxButton* selectionSortButton = new wxButton(frame, wxID_ANY, "Selection Sort");
        wxButton* mergeSortButton = new wxButton(frame, wxID_ANY, "Merge Sort");
        wxButton* quickSortButton = new wxButton(frame, wxID_ANY, "Quick Sort");
        wxButton* refreshButton = new wxButton(frame, wxID_ANY, "Refresh");

        buttonSizer->Add(numberInput, 0, wxALL, 5);
        buttonSizer->Add(addButton, 0, wxALL, 5);
        buttonSizer->Add(speedSlider, 0, wxALL, 5);
        buttonSizer->Add(bubbleSortButton, 1, wxEXPAND | wxALL, 5);
        buttonSizer->Add(insertionSortButton, 1, wxEXPAND | wxALL, 5);
        buttonSizer->Add(selectionSortButton, 1, wxEXPAND | wxALL, 5);
        buttonSizer->Add(mergeSortButton, 1, wxEXPAND | wxALL, 5);
        buttonSizer->Add(quickSortButton, 1, wxEXPAND | wxALL, 5);
        buttonSizer->Add(refreshButton, 1, wxEXPAND | wxALL, 5);

        sizer->Add(panel, 1, wxEXPAND);
        sizer->Add(buttonSizer, 0, wxEXPAND);
        frame->SetSizer(sizer);

        addButton->Bind(wxEVT_BUTTON, [panel, numberInput](wxCommandEvent&) {
            long value;
            if (numberInput->GetValue().ToLong(&value) && value >= 1 && value <= 100) {
                panel->AddBar(static_cast<int>(value)); 
            }
            });

        speedSlider->Bind(wxEVT_SLIDER, [panel, speedSlider](wxCommandEvent&) {
            panel->SetSpeed(speedSlider->GetValue()); 
            });

        bubbleSortButton->Bind(wxEVT_BUTTON, [panel](wxCommandEvent&) {
            panel->StartSorting(BubbleSort);
            });

        insertionSortButton->Bind(wxEVT_BUTTON, [panel](wxCommandEvent&) {
            panel->StartSorting(InsertionSort);
            });

        selectionSortButton->Bind(wxEVT_BUTTON, [panel](wxCommandEvent&) {
            panel->StartSorting(SelectionSort);
            });

        mergeSortButton->Bind(wxEVT_BUTTON, [panel](wxCommandEvent&) {
            panel->StartSorting(MergeSort);
            });

        quickSortButton->Bind(wxEVT_BUTTON, [panel](wxCommandEvent&) {
            panel->StartSorting(QuickSort);
            });

        refreshButton->Bind(wxEVT_BUTTON, [panel](wxCommandEvent&) {
            panel->RefreshArray();
            });

        frame->Show(true);
        return true;
    }
};

wxIMPLEMENT_APP(SortingVisualizerApp);
