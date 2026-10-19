#include "app/project_dialogs.h"

#include "core/media/project_folders.h"
#include "core/project/project_cleanup.h"
#include "core/project/project_file.h"

namespace
{
	const juce::String PROJECT_PATTERN = juce::String("*") + PROJECT_EXTENSION;
	constexpr int MAX_LISTED_FILES = 12;
	constexpr auto CLOCK_FORMAT = "%H:%M";
	constexpr auto DATE_FORMAT = "%d %b, %H:%M";
	constexpr auto CRASH_TITLE = "Voxalibera closed unexpectedly";

	juce::String describe_files(const juce::Array<juce::File>& files)
	{
		juce::StringArray lines;

		for (int index = 0; index < std::min(files.size(), MAX_LISTED_FILES); ++index)
			lines.add(files[index].getFileName() + "   " + describe_size(files[index].getSize()));

		if (files.size() > MAX_LISTED_FILES)
			lines.add("and " + juce::String(files.size() - MAX_LISTED_FILES) + " more");

		return lines.joinIntoString("\n");
	}

	ProjectDialogs::Next or_nothing(ProjectDialogs::Next next)
	{
		return next ? next : [] {};
	}

	juce::String describe_discard(const ProjectSession& session, const Project& project)
	{
		if (!session.is_saved() && is_unsaved_project_folder(project.folder))
			return "This project has never been saved. Don't Save deletes its recordings.";

		return "Your changes are lost if you don't save them.";
	}

	juce::File starting_folder()
	{
		const auto folder = ProjectSession::projects_folder();
		folder.createDirectory();
		return folder;
	}
}

ProjectDialogs::ProjectDialogs(AppContext& context, ShowMessage show_message) : context(context), show_message(std::move(show_message))
{
}

void ProjectDialogs::new_project()
{
	close_project_then([this] { context.session.start_new(); });
}

void ProjectDialogs::open_project()
{
	close_project_then([this] {
		chooser = std::make_unique<juce::FileChooser>("Open Project", starting_folder(), PROJECT_PATTERN);
		chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this](const juce::FileChooser& finished) {
			if (const auto file = finished.getResult(); file != juce::File())
				open_file(file);
		});
	});
}

void ProjectDialogs::open_recent(const juce::File& file)
{
	close_project_then([this, file] { open_file(file); });
}

void ProjectDialogs::open_file(const juce::File& file)
{
	if (crashed_in(file.getParentDirectory()))
		if (const auto autosaved = newer_autosave(file)) {
			offer_restore(file, *autosaved, {});
			return;
		}

	load(file, false);
}

void ProjectDialogs::load(const juce::File& file, bool restore_autosave)
{
	juce::StringArray problems;

	if (const auto result = context.session.open(file, problems, restore_autosave); result.failed()) {
		show_problem("Could not open the project", result.getErrorMessage());
		return;
	}

	if (!problems.isEmpty())
		show_problem("Opened with problems", problems.joinIntoString("\n"));
}

void ProjectDialogs::offer_restore(const juce::File& file, juce::Time autosaved, Next not_now)
{
	const auto message = "\"" + file.getFileNameWithoutExtension() + "\" was open when Voxalibera closed. An autosave from " + autosaved.formatted(CLOCK_FORMAT) + " has changes that were never saved.";
	std::vector<MessageChoice> choices;

	if (not_now)
		choices.push_back({"Not Now", not_now});

	choices.push_back({"Open Last Saved", [this, file] { load(file, false); }});
	choices.push_back({"Restore Autosave", [this, file] { load(file, true); }});
	show_message(CRASH_TITLE, message, std::move(choices));
}

void ProjectDialogs::check_recovery()
{
	for (const auto& file : context.session.recent.files)
		if (file.existsAsFile() && crashed_in(file.getParentDirectory())) {
			offer_reopen(file);
			return;
		}

	offer_unsaved(leftover_unsaved_sessions(), 0);
}

void ProjectDialogs::offer_reopen(const juce::File& file)
{
	const auto not_now = [this] { offer_unsaved(leftover_unsaved_sessions(), 0); };

	if (const auto autosaved = newer_autosave(file)) {
		offer_restore(file, *autosaved, not_now);
		return;
	}

	const auto message = "\"" + file.getFileNameWithoutExtension() + "\" was open when Voxalibera closed. Reopen it?";
	show_message(CRASH_TITLE, message, {{"Not Now", not_now}, {"Reopen", [this, file] { load(file, false); }}});
}

void ProjectDialogs::offer_unsaved(std::vector<UnsavedSession> sessions, size_t index)
{
	if (index >= sessions.size())
		return;

	const auto unsaved = sessions[index];
	const auto ask_next = [this, sessions, index] { offer_unsaved(sessions, index + 1); };
	const auto remove = [this, unsaved, ask_next] {
		unsaved.folder.deleteRecursively();
		log_event("Deleted the unsaved session in " + unsaved.folder.getFullPathName());
		ask_next();
	};

	auto message = "Found " + juce::String(unsaved.takes) + " recording(s) (" + describe_size(unsaved.bytes) + ") from an unsaved session on " + unsaved.time.formatted(DATE_FORMAT) + ".";

	if (!unsaved.has_autosave)
		message << " It was never autosaved, so each take comes back on its own track at the start.";

	show_message("Unsaved recordings", message, {{"Delete", remove}, {"Later", ask_next}, {"Recover", [this, unsaved] { context.session.recover(unsaved); }}});
}

void ProjectDialogs::save(Next next)
{
	if (!context.session.is_saved() || context.session.is_read_only()) {
		save_as(std::move(next));
		return;
	}

	if (const auto result = context.session.save(); result.failed()) {
		show_problem("Could not save the project", result.getErrorMessage());
		return;
	}

	if (next)
		next();
}

void ProjectDialogs::save_as(Next next)
{
	const auto initial = context.session.is_saved() ? context.session.project_file() : starting_folder().getChildFile(context.session.name() + PROJECT_EXTENSION);
	const auto flags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting;

	chooser = std::make_unique<juce::FileChooser>("Save Project As", initial, PROJECT_PATTERN);
	chooser->launchAsync(flags, [this, next](const juce::FileChooser& finished) {
		const auto file = finished.getResult();

		if (file == juce::File())
			return;

		if (const auto result = context.session.save_as(file.withFileExtension(PROJECT_EXTENSION)); result.failed()) {
			show_problem("Could not save the project", result.getErrorMessage());
			return;
		}

		if (next)
			next();
	});
}

void ProjectDialogs::close_project_then(Next next)
{
	const auto then = [this, next] { hint_cleanup_then(next); };

	if (!context.session.has_unsaved_changes()) {
		then();
		return;
	}

	const auto discard = [this, next] {
		context.session.discard_changes();
		next();
	};
	const auto save_first = [this, then] { save(then); };
	const auto title = "Save changes to \"" + context.session.name() + "\"?";

	show_message(title, describe_discard(context.session, context.project), {{"Don't Save", discard}, {"Cancel", {}}, {"Save", save_first}});
}

void ProjectDialogs::clean_up(Next next)
{
	next = or_nothing(std::move(next));
	context.plugins.store_states(context.project);
	const auto unused = unused_media(context.project);

	if (unused.isEmpty()) {
		show_message("Clean Up Project", "No unused audio files in this project.", {{"OK", next}});
		return;
	}

	if (context.session.is_saved() && context.session.has_unsaved_changes()) {
		const auto message = "The project has unsaved changes. Save it first, so the saved project never points at deleted files.";
		show_message("Clean Up Project", message, {{"Cancel", next}, {"Save and Continue", [this, next] { save([this, next] { clean_up(next); }); }}});
		return;
	}

	confirm_clean_up(unused, next);
}

void ProjectDialogs::confirm_clean_up(const juce::Array<juce::File>& unused, Next next)
{
	const auto message = juce::String(unused.size()) + " audio file(s) nothing in the project uses, " + describe_size(total_size(unused)) + " in total:\n\n" + describe_files(unused) + "\n\nDeleting them can't be undone, and it clears the undo history.";
	const auto remove = [this, unused, next] {
		context.session.clean_up(unused);
		next();
	};

	show_message("Clean Up Project", message, {{"Cancel", next}, {"Delete Files", remove}});
}

void ProjectDialogs::hint_cleanup_then(Next next)
{
	const auto& file = context.session.project_file();

	if (!context.session.is_saved() || context.session.has_unsaved_changes()) {
		next();
		return;
	}

	const auto bytes = total_size(unused_media(context.project));

	if (!should_hint_cleanup(file, bytes)) {
		next();
		return;
	}

	remember_cleanup_hint(file, bytes);
	const auto message = "This project has " + describe_size(bytes) + " of takes no clip uses. File > Clean Up Project removes them.";
	show_message("Unused takes", message, {{"Not Now", next}, {"Clean Up", [this, next] { clean_up(next); }}});
}

void ProjectDialogs::show_problem(const juce::String& title, const juce::String& message)
{
	show_message(title, message, {{"OK", {}}});
}
