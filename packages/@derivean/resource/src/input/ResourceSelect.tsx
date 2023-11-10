import {type ValuesSchema}      from "@use-pico/form";
import {
    type ComponentProps,
    useCallback
}                               from "react";
import {ResourceInline}         from "../inline/ResourceInline";
import {ResourceTable}          from "../table/ResourceTable";
import {ResourceSelectionInput} from "./ResourceSelectionInput";

export namespace ResourceSelect {
    export interface Props<
        TValuesSchema extends ValuesSchema,
    > extends Omit<
        ComponentProps<ResourceSelectionInput<TValuesSchema>["single"]>,
        "Selector" | "Item"
    > {
        tableProps?: Partial<ComponentProps<typeof ResourceTable>>;
    }
}

export const ResourceSelect = <
    TValuesSchema extends ValuesSchema,
>(
    {
        tableProps,
        ...props
    }: ResourceSelect.Props<TValuesSchema>
) => {
    return <ResourceSelectionInput.single
        Selector={useCallback(() => <ResourceTable withLinkLock {...tableProps}/>, [])}
        Item={ResourceInline}
        {...props}
    />;
};
