#ifndef SYNTAX_HIGHLIGHT_H
#define SYNTAX_HIGHLIGHT_H

#include <gtk/gtk.h>
#include <string>
#include <vector>
#include <regex>

// Forward declare Grammar and variable_list from main.cpp / grammar.cpp
class Grammar;
extern Grammar *grammar;
extern std::vector<class Variable *> variable_list;

// Apply a named tag to a buffer range (safe helper)
static void apply_tag_safe(GtkTextBuffer *buffer, const char *tag_name,
                           const GtkTextIter &start, const GtkTextIter &end) {
    GtkTextTagTable *table = gtk_text_buffer_get_tag_table(buffer);
    if (!table) return;
    GtkTextTag *tag = gtk_text_tag_table_lookup(table, tag_name);
    if (!tag) return;
    gtk_text_buffer_apply_tag_by_name(buffer, tag_name, &start, &end);
}

static bool is_ident_char(gunichar c) {
    return g_unichar_isalnum(c) || c == '_';
}

static bool is_standalone_match(const GtkTextIter &match_start, const GtkTextIter &match_end) {
    bool left_ok = true;
    bool right_ok = true;
    if (!gtk_text_iter_is_start(&match_start)) {
        GtkTextIter prev = match_start;
        gtk_text_iter_backward_char(&prev);
        left_ok = !is_ident_char(gtk_text_iter_get_char(&prev));
    }
    if (!gtk_text_iter_is_end(&match_end)) {
        GtkTextIter next = match_end;
        right_ok = !is_ident_char(gtk_text_iter_get_char(&next));
    }
    return left_ok && right_ok;
}

static void apply_regex_matches(GtkTextBuffer *buffer,
                                const char *tag_name,
                                const GtkTextIter &line_start,
                                const std::string &line_text,
                                const std::regex &pattern,
                                std::size_t capture_index = 0) {
    auto search_start = line_text.cbegin();
    std::smatch match;
    while (std::regex_search(search_start, line_text.cend(), match, pattern)) {
        const auto &group = match[capture_index];
        if (group.matched) {
            const int start_offset = static_cast<int>(group.first - line_text.cbegin());
            const int end_offset = static_cast<int>(group.second - line_text.cbegin());
            GtkTextIter tag_start = line_start;
            gtk_text_iter_set_offset(&tag_start, gtk_text_iter_get_offset(&line_start) + start_offset);
            GtkTextIter tag_end = line_start;
            gtk_text_iter_set_offset(&tag_end, gtk_text_iter_get_offset(&line_start) + end_offset);
            if (is_standalone_match(tag_start, tag_end)) {
                apply_tag_safe(buffer, tag_name, tag_start, tag_end);
            }
        }
        search_start = match.suffix().first;
    }
}

// Update syntax highlighting using the Grammar instance. This highlights:
// - rule names (blue)
// - variables (green)
// - numbers (orange)
// - operators / single-letter tokens (bold)
// - comments (gray, lines starting with # or //)
static void update_syntax_highlighting(GtkTextBuffer *buffer) {
    if (!buffer) return;

    // Ensure tags exist (create minimal tags if caller didn't)
    GtkTextTagTable *table = gtk_text_buffer_get_tag_table(buffer);
    if (!table) return;

    if (!gtk_text_tag_table_lookup(table, "rule"))
        gtk_text_buffer_create_tag(buffer, "rule", "foreground", "#1E90FF", NULL);
    if (!gtk_text_tag_table_lookup(table, "variable"))
        gtk_text_buffer_create_tag(buffer, "variable", "foreground", "#2E8B57", NULL);
    if (!gtk_text_tag_table_lookup(table, "number"))
        gtk_text_buffer_create_tag(buffer, "number", "foreground", "#FF8C00", NULL);
    if (!gtk_text_tag_table_lookup(table, "boldtok"))
        gtk_text_buffer_create_tag(buffer, "boldtok", "weight", PANGO_WEIGHT_BOLD, NULL);
    if (!gtk_text_tag_table_lookup(table, "comment"))
        gtk_text_buffer_create_tag(buffer, "comment", "foreground", "#808080", NULL);
    if (!gtk_text_tag_table_lookup(table, "operator"))
        gtk_text_buffer_create_tag(buffer, "operator", "foreground", "#7F3FBF", "weight", PANGO_WEIGHT_BOLD, NULL);

    // Get full buffer bounds
    GtkTextIter start, end;
    gtk_text_buffer_get_start_iter(buffer, &start);
    gtk_text_buffer_get_end_iter(buffer, &end);

    // Remove all tags first
    gtk_text_buffer_remove_all_tags(buffer, &start, &end);

    const std::regex rule_re("^\\s*([A-Za-z_][A-Za-z0-9_]*)\\b.*->");
    const std::regex variable_decl_re("\\bR\\*?\\s+([A-Za-z_][A-Za-z0-9_]*)\\b");
    const std::regex number_re("[-+]?(\\d+\\.\\d+|\\d+|\\.\\d+)");
    const std::regex operator_re("(->|\\||\\[|\\]|\\{|\\}|\\(|\\)|\\b(R\\*?|S|T|I|A|D)\\b)");

    // Text-driven pass by line so highlighting stays correct even before regeneration.
    GtkTextIter iter = start;
    while (!gtk_text_iter_is_end(&iter)) {
        GtkTextIter line_end = iter;
        gtk_text_iter_forward_to_line_end(&line_end);

        gchar *line_text = gtk_text_buffer_get_text(buffer, &iter, &line_end, FALSE);
        if (line_text) {
            std::string line(line_text);
            std::size_t comment_pos = line.find('#');
            std::size_t slash_comment_pos = line.find("//");
            if (slash_comment_pos != std::string::npos &&
                (comment_pos == std::string::npos || slash_comment_pos < comment_pos)) {
                comment_pos = slash_comment_pos;
            }

            std::string code_part = line;
            if (comment_pos != std::string::npos) {
                GtkTextIter comment_start = iter;
                gtk_text_iter_set_offset(&comment_start, gtk_text_iter_get_offset(&iter) + static_cast<int>(comment_pos));
                apply_tag_safe(buffer, "comment", comment_start, line_end);
                code_part = line.substr(0, comment_pos);
            }

            apply_regex_matches(buffer, "rule", iter, code_part, rule_re, 1);
            apply_regex_matches(buffer, "variable", iter, code_part, variable_decl_re, 1);
            apply_regex_matches(buffer, "number", iter, code_part, number_re, 0);
            apply_regex_matches(buffer, "operator", iter, code_part, operator_re, 0);
            g_free(line_text);
        }

        if (gtk_text_iter_equal(&line_end, &end)) break;
        gtk_text_iter_forward_line(&iter);
    }
}

#endif // SYNTAX_HIGHLIGHT_H
