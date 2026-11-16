function flush_file(    executed_lines, executed_branches, taken_branches) {
    if (file == "") {
        return;
    }

    if (lines_total > 0) {
        executed_lines = int(lines_pct * lines_total / 100.0 + 0.5);
        total_lines_exec += executed_lines;
        total_lines += lines_total;
    } else {
        executed_lines = 0;
    }

    if (branches_total > 0) {
        executed_branches = int(branches_pct * branches_total / 100.0 + 0.5);
        taken_branches = int(taken_pct * branches_total / 100.0 + 0.5);
        total_branches_exec += executed_branches;
        total_branches_taken += taken_branches;
        total_branches += branches_total;
        printf "%-28s %6.2f%% (%3d/%3d)  %6.2f%% (%3d/%3d)  %6.2f%% (%3d)\n", file, lines_pct, executed_lines, lines_total, branches_pct, executed_branches, branches_total, taken_pct, taken_branches;
    } else {
        printf "%-28s %6.2f%% (%3d/%3d)  %10s  %10s\n", file, lines_pct, executed_lines, lines_total, "n/a", "n/a";
    }
}

BEGIN {
    print "Coverage report";
    print "================================================================================";
    printf "%-28s %17s %17s %12s\n", "File", "Lines", "Branches", "Taken";
    print "--------------------------------------------------------------------------------";
}

/^File / {
    flush_file();
    file = $0;
    sub(/^File /, "", file);
    gsub(/'/, "", file);
    in_file = 1;
    lines_pct = 0;
    lines_total = 0;
    branches_pct = 0;
    branches_total = 0;
    taken_pct = 0;
    next;
}

/^Creating / {
    in_file = 0;
    next;
}

/^Lines executed:/ {
    if (!in_file) {
        next;
    }

    value = $0;
    sub(/^Lines executed:/, "", value);
    sub(/%.*/, "", value);
    lines_pct = value + 0;

    value = $0;
    sub(/^.* of /, "", value);
    lines_total = value + 0;
    next;
}

/^Branches executed:/ {
    if (!in_file) {
        next;
    }

    value = $0;
    sub(/^Branches executed:/, "", value);
    sub(/%.*/, "", value);
    branches_pct = value + 0;

    value = $0;
    sub(/^.* of /, "", value);
    branches_total = value + 0;
    next;
}

/^Taken at least once:/ {
    if (!in_file) {
        next;
    }

    value = $0;
    sub(/^Taken at least once:/, "", value);
    sub(/%.*/, "", value);
    taken_pct = value + 0;
    next;
}

END {
    flush_file();
    print "================================================================================";

    if (total_lines > 0) {
        total_lines_pct = 100.0 * total_lines_exec / total_lines;
    } else {
        total_lines_pct = 0.0;
    }

    if (total_branches > 0) {
        total_branches_pct = 100.0 * total_branches_exec / total_branches;
        total_taken_pct = 100.0 * total_branches_taken / total_branches;
    } else {
        total_branches_pct = 0.0;
        total_taken_pct = 0.0;
    }

    printf "%-28s %6.2f%% (%3d/%3d)  %6.2f%% (%3d/%3d)  %6.2f%% (%3d)\n", "TOTAL", total_lines_pct, total_lines_exec, total_lines, total_branches_pct, total_branches_exec, total_branches, total_taken_pct, total_branches_taken;
}