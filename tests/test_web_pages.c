/*
 * test_web_pages.c
 * Tests des pages web statiques : CSP, absence de script inline, point
 * d'entrée et heure d'aperçu.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "support/test_runner.h"

#define WEB_INDEX_HTML "web/index.html"
#define WEB_APP_JS "web/js/app.js"
#define WEB_PREVIEW_JS "web/js/preview.js"

// Lit un fichier texte en mémoire ; l'appelant libère le tampon retourné.
static char *read_text_file(const char *path) {
  FILE *file = fopen(path, "rb");
  long size;
  char *buffer;

  if (file == NULL) {
    return NULL;
  }
  if (fseek(file, 0, SEEK_END) != 0) {
    (void)fclose(file);
    return NULL;
  }
  size = ftell(file);
  if (size < 0 || fseek(file, 0, SEEK_SET) != 0) {
    (void)fclose(file);
    return NULL;
  }

  buffer = malloc((size_t)size + 1);
  if (buffer == NULL) {
    (void)fclose(file);
    return NULL;
  }
  if (fread(buffer, 1, (size_t)size, file) != (size_t)size) {
    free(buffer);
    (void)fclose(file);
    return NULL;
  }
  buffer[size] = '\0';
  (void)fclose(file);
  return buffer;
}

static void assert_contains(const char *text, const char *expected) {
  if (strstr(text, expected) == NULL) {
    runner_context_fail(__FILE__, __LINE__, "texte attendu absent : \"%s\"",
                        expected);
  }
}

static void assert_not_contains(const char *text, const char *unexpected) {
  if (strstr(text, unexpected) != NULL) {
    runner_context_fail(__FILE__, __LINE__, "texte interdit présent : \"%s\"",
                        unexpected);
  }
}

// Vérifie que la page n'utilise aucun gestionnaire ou style en ligne : la
// politique de sécurité (script-src et style-src 'self') les bloquerait.
static void test_web_page_has_no_inline_script(void) {
  // Given
  const char *const inline_markers[] = {
      "style=\"",  "onclick=",  "onload=",
      "onsubmit=", "onchange=", "javascript:",
  };
  const size_t marker_count =
      sizeof(inline_markers) / sizeof(inline_markers[0]);

  // When
  char *html = read_text_file(WEB_INDEX_HTML);

  // Then
  TEST_ASSERT_TRUE(html != NULL);
  if (html != NULL) {
    for (size_t i = 0; i < marker_count; ++i) {
      assert_not_contains(html, inline_markers[i]);
    }
    free(html);
  }
}

// Vérifie que la politique de sécurité de la page autorise le chargement local
// du module WebAssembly : garde-fou contre une directive qui le bloquerait.
static void test_web_page_csp_authorizes_local_wasm(void) {
  // Given
  const char *const forbidden_directives[] = {
      "connect-src 'none'",
      "default-src *",
  };
  const size_t forbidden_count =
      sizeof(forbidden_directives) / sizeof(forbidden_directives[0]);

  // When
  char *html = read_text_file(WEB_INDEX_HTML);

  // Then
  TEST_ASSERT_TRUE(html != NULL);
  if (html != NULL) {
    assert_contains(html, "connect-src 'self'");
    assert_contains(html, "'wasm-unsafe-eval'");
    assert_contains(html, "object-src 'none'");
    for (size_t i = 0; i < forbidden_count; ++i) {
      assert_not_contains(html, forbidden_directives[i]);
    }
    free(html);
  }
}

// Vérifie que la page charge un point d'entrée de module et que celui-ci
// importe le binaire WebAssembly : un renommage devient une erreur de test.
// Le point d'entrée porte une version de cache : sans elle, un navigateur
// continue d'exécuter l'ancien module après une modification de app.js.
static void test_web_page_loads_module_entry_point(void) {
  // Given
  const char *const expected_entry_point = "type=\"module\"";

  // When
  char *html = read_text_file(WEB_INDEX_HTML);
  char *app = read_text_file(WEB_APP_JS);

  // Then
  TEST_ASSERT_TRUE(html != NULL);
  TEST_ASSERT_TRUE(app != NULL);
  if (html != NULL) {
    assert_contains(html, expected_entry_point);
    assert_contains(html, "app.js");
    assert_contains(html, "app.js?v=");
    free(html);
  }
  if (app != NULL) {
    assert_contains(app, "moon_core.mjs");
    free(app);
  }
}

// Vérifie que la page expose l'instant d'aperçu et que le calcul passe par les
// exports datés : un renommage du champ, du module ou d'un export devient une
// erreur de test.
static void test_web_page_uses_preview_time(void) {
  // Given
  const char *const html_markers[] = {
      "id=\"time-input\"",
      "id=\"now-button\"",
      "id=\"preview-note\"",
  };
  const char *const app_markers[] = {
      "setNow",
      "createCityPicker",
      "readSettings",
  };
  const char *const preview_markers[] = {
      "readPreview",
      "_moon_web_ascii_into",
      "_moon_web_phase_at",
      "_moon_web_illumination_at",
      "_moon_web_bright_limb_angle_at",
  };
  const size_t html_marker_count =
      sizeof(html_markers) / sizeof(html_markers[0]);
  const size_t app_marker_count = sizeof(app_markers) / sizeof(app_markers[0]);
  const size_t preview_marker_count =
      sizeof(preview_markers) / sizeof(preview_markers[0]);

  // When
  char *html = read_text_file(WEB_INDEX_HTML);
  char *app = read_text_file(WEB_APP_JS);
  char *preview = read_text_file(WEB_PREVIEW_JS);

  // Then
  TEST_ASSERT_TRUE(html != NULL);
  TEST_ASSERT_TRUE(app != NULL);
  TEST_ASSERT_TRUE(preview != NULL);
  if (html != NULL) {
    for (size_t i = 0; i < html_marker_count; ++i) {
      assert_contains(html, html_markers[i]);
    }
    free(html);
  }
  if (app != NULL) {
    for (size_t i = 0; i < app_marker_count; ++i) {
      assert_contains(app, app_markers[i]);
    }
    free(app);
  }
  if (preview != NULL) {
    for (size_t i = 0; i < preview_marker_count; ++i) {
      assert_contains(preview, preview_markers[i]);
    }
    free(preview);
  }
}

static const TestCase test_cases[] = {
    {"test_web_page_has_no_inline_script", test_web_page_has_no_inline_script},
    {"test_web_page_csp_authorizes_local_wasm",
     test_web_page_csp_authorizes_local_wasm},
    {"test_web_page_loads_module_entry_point",
     test_web_page_loads_module_entry_point},
    {"test_web_page_uses_preview_time", test_web_page_uses_preview_time},
};

int main(void) {
  return runner_run("web_pages", test_cases, TEST_CASE_COUNT(test_cases));
}