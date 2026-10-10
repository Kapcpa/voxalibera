#include "app/project_dialogs.h"

#include "core/media/take_files.h"
#include "core/project/project_file.h"

namespace
{
	const juce::String PROJECT_PATTERN = juce::String("*") + PROJECT_EXTENSION;

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
	juce::StringArray problems;

	if (const auto result = context.session.open(file, problems); result.failed()) {
		show_problem("Could not open the project", result.getErrorMessage());
		return;
	}

	if (!problems.isEmpty())
		show_problem("Opened with problems", problems.joinIntoString("\n"));
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
	if (!context.session.has_unsaved_changes()) {
		next();
		return;
	}

	const auto discard = [this, next] {
		context.session.discard_unsaved_audio();
		next();
	};
	const auto save_first = [this, next] { save(next); };
	const auto title = "Save changes to \"" + context.session.name() + "\"?";

	show_message(title, describe_discard(context.session, context.project), {{"Don't Save", discard}, {"Cancel", {}}, {"Save", save_first}});
}

void ProjectDialogs::show_problem(const juce::String& title, const juce::String& message)
{
	show_message(title, message, {{"OK", {}}});
}
