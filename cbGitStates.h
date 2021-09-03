#ifndef CBGITSTATES_H
#define CBGITSTATES_H

class cbGitFileState
{
public:
    enum state_t
    {
        unmodified,
        newfile,
        modified,
        deleted,
        renamed,
        unreadable,
        ignored,
        conflicted,

    };
    state_t state;
    cbGitFileState():
        state(unmodified)
    {}
};

#endif

