import {DateTimeInline}  from "@use-pico2/i18n";
import {Table}           from "@use-pico2/table";
import {toHumanBytes}    from "@use-pico2/utils";
import {
    type FC,
    type PropsWithChildren
}                        from "react";
import {FileQuerySchema} from "../schema/FileQuerySchema";
import {FileSchema}      from "../schema/FileSchema";

export namespace FileTable {
    export type Columns =
        | "name"
        | "path"
        | "mime"
        | "size"
        | "created"
        | "ttl"
        | "native";

    export interface Props extends Omit<
        Table.Props<
            Columns,
            FileSchema,
            FileQuerySchema
        >,
        "columns"> {
        DownloadButton: FC<PropsWithChildren<{
            file: FileSchema.Type;
        }>>;
    }
}

export const FileTable: FC<FileTable.Props> = (
    {
        DownloadButton,
        ...props
    }) => {
    return <Table
        scrollWidth={2400}
        columns={{
            name:    {
                render: ({item}) => <DownloadButton
                    file={item}
                >
                    {item.name}
                </DownloadButton>,
            },
            path:    {
                withFilter: {
                    isFilter: filter => filter?.path !== undefined,
                    onFilter: ({
                                   shallowFilter,
                                   item,
                               }) => {
                        shallowFilter({
                            path: item.path,
                        });
                    },
                    onClear:  ({shallowFilter}) => {
                        shallowFilter({
                            path: undefined,
                        });
                    },
                },
                render:     ({item}) => item.path,
                width:      14,
            },
            size:    {
                render: ({item}) => toHumanBytes(item.size),
                width:  8,
            },
            mime:    {
                withFilter: {
                    isFilter: filter => filter?.mime !== undefined,
                    onFilter: ({
                                   shallowFilter,
                                   item,
                               }) => {
                        shallowFilter({
                            mime: item.mime,
                        });
                    },
                    onClear:  ({shallowFilter}) => {
                        shallowFilter({
                            mime: undefined,
                        });
                    },
                },
                render:     ({item}) => item.mime,
                width:      14,
            },
            created: {
                render: ({item}) => <DateTimeInline date={item.created}/>,
                width:  10,
            },
            ttl:     {
                render: ({item}) => item.ttl,
                width:  10,
            },
            native:  {
                render: ({item}) => item.native,
                width:  50,
            },
        }}
        {...props}
    />;
};
