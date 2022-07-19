# Wide Column Table Notation

## Motivation


GNU's [recfiles](https://www.gnu.org/software/recutils/manual/index.html#Top) are awesome. A similar format might be well suited to storing datomic-style datalog data as plain text.

The idea is that by restraining the syntax and limiting representation to just a single datom, we can make reading and writing the file *really* fast, while making it easy to read.

## Description

If a byte order mark is encountered as the first byte, it is ignored.

A WCTN document consists of zero or more forms.

Each form starts with one or more non-empty lines containing no tab characters, followed by at least one empty line. The string on the first line is the title. Subsequent lines, if any, are called the form's doc string or description. The title and description together is called the preamble of the form.

Following the preamble is a sequence of zero or more records. Each record consists of a sequence of fields followed by at least one empty line.

Each field consists of the start line and subsequent value continuation lines.

On the start line, there is first a string free of tabs, followed by the tab character, followed by more non-tab characters. The string before the tab is the key. The string after the tab, up to and including the ending line delimiter of the line, is the value string. Neither strings may be empty and neither may start with the space character.

Subsequent lines, if they start with the tab character, are value continuation lines. Everything in these lines, including the ending line delimiter, is concatenated to the value string on the start line without modification.

The first line not to start with the tab character is simply taken as the start of the next field in the record.

Records end when one or more empty lines are encountered.

In consequence of the above rule, it is impossible to have a record with fewer than at least one field.



## Application to Clojure Datalog

Each record could simply represent a datom. Keys could be keywords or symbols and values could be EDN.

## ABNF

```
WCTN = *1%xFEFF        ; bom check
       *( form )

; Form Rules
form = preamble
       *1line-delimiter
       *record


preamble = *1( notab
               line-delimiter )

; Record Rules
record = 1*field
         1*line-delimiter

field = notab
        1*( tab
            content
            line-delimiter )

tab = %x09             ; \t
notab = %x20-10FFFF
content = notab
        / tab

line-delimiter = %x0D %x0A
               / %x0D
               / %x0A
```