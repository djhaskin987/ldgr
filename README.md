# Line Delimited General Records

Think of it as a TSV of sorts for wide-column tables.

## Example

TODO: Put example document here.

## Motivation

GNU's
[recfiles](https://www.gnu.org/software/recutils/manual/index.html#Top)
are awesome. A similar format might be well suited to storing
wide-column database data as plain text.

The idea is that by restraining the syntax and limiting representation,
we can make reading and writing the file *really* fast, while making it easy to
read for humans.

## Description

"General" in the name is meant to invoke the idea of "wide" or also of
"approximate". A LDGR file represents a wide-column table.

If a byte order mark is encountered as the first byte, it is ignored.

A LDGR file consists of a number of stanzas. A stanza is a series of one
or more non-empty lines. Eacy line must have at least one tab
character.

On the start line of a stanza, there is first a string free of tabs,
followed by the tab character, followed by more non-tab characters. The
string before the tab is the key. The string after the tab, up to and
including the ending line delimiter of the line, is the value string.
Neither strings may be empty and neither may start with the space
character.

Subsequent lines, if they start with the tab character, are value
continuation lines. Everything in these lines after the first tab
characters, including the ending line delimiter, is concatenated to the
value string on the start line without modification.

The first line not to start with the tab character is simply taken as
he start of the next field in the record.

Records end when one or more empty lines are encountered.

In consequence of the above rule, it is impossible to have a record with
fewer than at least one field.

Each record can be thought of as a row in a wide-column table.

The first record in the LDGR document is taken to have schema
information about the table represented. Keys in this record should
(*generally*, wink wink) be found in subsequent records. In particular,
the first key in the first record must appear as the first record in
subsequent rows.

The first key in each record must be the same in each record, and is
taken to be the primary key of the wide column table.

If keys are in the schema and are general to all records in the table
(at least generally), then they should appear in the same order as they
appear in the schema. Other keys may also exist in records, with
un-schema-'d keys appearing after those that are in the schema in each
record.


## Implementations

For some example implementations of this format, see the
"implementations" folder in this repository.

## ABNF

```
LDGR = *1%xFEFF        ; bom check
       *record

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