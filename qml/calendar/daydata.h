#ifndef DAYDATA_H
#define DAYDATA_H

class DayData
{
public:
    bool isPreviousMonth;
    bool isCurrentMonth;
    bool isNextMonth;
    bool containsHolidayItems;
    bool containsEventItems;
    bool containsTodoItems;
    bool containsJournalItems;
    int dayNumber;
};


#endif // DAYDATA_H
