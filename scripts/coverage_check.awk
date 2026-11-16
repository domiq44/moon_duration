# Vérifie que la couverture globale atteint les seuils donnés.
#
# Usage : awk -v min_lines=90 -v min_branches=75 -f coverage_check.awk summary.txt
# Sortie : un résumé, et un code retour non nul si un seuil n'est pas atteint.

function percent(part, total) {
    if (total <= 0) {
        return 0.0;
    }
    return 100.0 * part / total;
}

function flush_file(    executed) {
    if (file == "") {
        return;
    }

    if (lines_total > 0) {
        executed = int(lines_pct * lines_total / 100.0 + 0.5);
        total_lines_exec += executed;
        total_lines += lines_total;
    }

    if (branches_total > 0) {
        # Les branches « prises au moins une fois » sont la mesure usuelle :
        # « Branches executed » ne dit que le code a été atteint.
        executed = int(taken_pct * branches_total / 100.0 + 0.5);
        total_branches_exec += executed;
        total_branches += branches_total;
    }
}

BEGIN {
    if (min_lines == "") {
        min_lines = 0;
    }
    if (min_branches == "") {
        min_branches = 0;
    }
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

    lines_pct_total = percent(total_lines_exec, total_lines);
    branches_pct_total = percent(total_branches_exec, total_branches);

    printf "Couverture : lignes %.2f %% (%d/%d, seuil %.2f %%), branches prises %.2f %% (%d/%d, seuil %.2f %%)\n",
           lines_pct_total, total_lines_exec, total_lines, min_lines,
           branches_pct_total, total_branches_exec, total_branches, min_branches;

    if (lines_pct_total < min_lines || branches_pct_total < min_branches) {
        print "Échec : couverture inférieure au seuil." > "/dev/stderr";
        exit 1;
    }
}