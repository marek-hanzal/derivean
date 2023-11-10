import {type ValuesSchema}          from "@use-pico/form";
import {
    type ComponentProps,
    useCallback
}                                   from "react";
import {ResourceTypeInline}         from "../inline/ResourceTypeInline";
import {ResourceTypeTable}          from "../table/ResourceTypeTable";
import {ResourceTypeSelectionInput} from "./ResourceTypeSelectionInput";

export namespace ResourceTypeSelect {
    export interface Props<
        TValuesSchema extends ValuesSchema,
    > extends Omit<
        ComponentProps<ResourceTypeSelectionInput<TValuesSchema>["single"]>,
        "Selector" | "Item"
    > {
        tableProps?: Partial<ComponentProps<typeof ResourceTypeTable>>;
    }
}

export const ResourceTypeSelect = <
    TValuesSchema extends ValuesSchema,
>(
    {
        tableProps,
        ...props
    }: ResourceTypeSelect.Props<TValuesSchema>
) => {
    return <ResourceTypeSelectionInput.single
        Selector={useCallback(() => <ResourceTypeTable withLinkLock {...tableProps}/>, [])}
        Item={ResourceTypeInline}
        {...props}
    />;
};
