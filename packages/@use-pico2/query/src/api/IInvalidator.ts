import {type QueryKey}   from "@tanstack/react-query/build/modern";
import {type IWithQuery} from "./IWithQuery";

export interface IInvalidator {
    readonly key: QueryKey;

    invalidator?(props: IWithQuery.InvalidatorProps): Promise<void>;
}

export namespace IInvalidator {
    export type Result = () => Promise<void>;
}
