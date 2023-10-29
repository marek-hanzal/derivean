import {OrderSchema} from "@use-pico2/query";
import {
    type PicoSchema,
    schema
}                    from "@use-pico2/schema";

export const FileOrderBySchema = schema(z => z.record(z.enum(["name", "path"]), OrderSchema));
export type FileOrderBySchema = typeof FileOrderBySchema;
export namespace FileOrderBySchema {
    export type Type = PicoSchema.Output<FileOrderBySchema>;
}
