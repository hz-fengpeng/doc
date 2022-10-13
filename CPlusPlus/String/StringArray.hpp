#pragma once
#include <vector>
#include "String.hpp"



class StringArray
{
    public:
        StringArray(){}

        ~StringArray(){}

        void addString(const String& s)
        {
            strArray.push_back(s);
            std::cout << "add ---" << std::endl;
        }

        void printArray()
        {
            for(auto it = strArray.begin(); it != strArray.end(); it++)
            {
                std::cout << *it <<" ";
            }
            std::cout << std::endl;
        }

        String& getByIndex(int i)
        {
            return strArray[i];
        }


    private:
        std::vector<String> strArray;

};