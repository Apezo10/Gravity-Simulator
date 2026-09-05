#include <iostream>
using namespace std;


void multiplyArray(int* group1, int* group2, int* newGroup) {

    for (int i = 0; i < 5; i++) {
        newGroup[i] = group1[i] * group2[i];
        cout << newGroup[i] << ", ";
    }
}

void getMinAndMax(int newArray[], int size, int* min, int* max) {
    for (int i = 0; i < size; i++) {
        if (*min > newArray[i]) {
            *min = newArray[i];
        }

        if (*max < newArray[i]) {
            *max = newArray[i];
        }
    }
}

void getArray() {
    int size{};
    cout << "Size of array: ";
    cin >> size;

    //allocate dynamic memorey and allows user to choose size of array
    int* array = new int[size];

    for (int i = 0; i < size; i++) {
        cout << "Array[" << i << "] = ";
        cin >> array[i];
    }

    //Print array
    cout << "Array = ";

    for (int i = 0; i<size; i++) {
        cout << array[i] << " ";
    }

    //Free up memory
    delete[]array;
    array = NULL;
}

int main() {
    getArray();
}