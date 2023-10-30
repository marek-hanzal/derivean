import {orderByOf}       from "@use-pico2/query";
import {type PicoSchema} from "@use-pico2/schema";

export const FileOrderBySchema = orderByOf(["name", "path"]);
export type FileOrderBySchema = typeof FileOrderBySchema;
export namespace FileOrderBySchema {
    export type Type = PicoSchema.Output<FileOrderBySchema>;
}
