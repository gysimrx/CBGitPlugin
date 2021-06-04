#ifndef CBGITSTATES_H
#define CBGITSTATES_H

class cbGitFileState
{
public:
    enum state_t
    {
        unmodified,
        fnew, //because of new Keyword
        modified,
        deleted,
        renamed,
        unreadable,
        ignored,
        conflicted,

    };
    state_t state;

};


#endif // CBGITSTATES_H
