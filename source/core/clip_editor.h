#pragma once

#include "core/project/edit_history.h"
#include "core/selection.h"

struct ClipMove
{
	std::int64_t delta = 0;
	int track_offset = 0;

	bool is_empty() const { return delta == 0 && track_offset == 0; }
};

class ClipEditor
{
public:
	ClipEditor(Project& project, EditHistory& edits, Selection& selection);

	ClipMove limit_move(ClipMove move) const;
	void move_selected(ClipMove move);
	void remove_selected();

private:
	Project& project;
	EditHistory& edits;
	Selection& selection;
};
