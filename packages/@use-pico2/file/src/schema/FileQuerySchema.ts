import {withQuerySchema}   from "@use-pico2/query";
import {type PicoSchema}   from "@use-pico2/schema";
import {FileFilterSchema}  from "./FileFilterSchema";
import {FileOrderBySchema} from "./FileOrderBySchema";

export const FileQuerySchema = withQuerySchema({
    filter:  FileFilterSchema,
    orderBy: FileOrderBySchema,
});
export type FileQuerySchema = typeof FileQuerySchema;
export namespace FileQuerySchema {
    export type Type = PicoSchema.Output<FileQuerySchema>;
}
