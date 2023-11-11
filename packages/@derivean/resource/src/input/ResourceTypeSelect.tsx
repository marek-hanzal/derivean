import {
    type ValuesSchema,
    withSourceQueryInput
}                                   from "@use-pico/form";
import {
    type ComponentProps,
    useCallback
}                                   from "react";
import {ResourceTypeInline}         from "../inline/ResourceTypeInline";
import {ResourceTypeQueryStore}     from "../query/ResourceTypeQueryStore";
import {ResourceTypeRpc}            from "../rpc/ResourceTypeRpc";
import {ResourceTypeSelectionStore} from "../store/ResourceTypeSelectionStore";
import {ResourceTypeTable}          from "../table/ResourceTypeTable";

const ResourceTypeQueryInput = withSourceQueryInput({
    withQueryStore:  ResourceTypeQueryStore,
    withSourceQuery: ResourceTypeRpc.query,
    SelectionStore:  ResourceTypeSelectionStore.single,
});
type ResourceTypeQueryInput<
    TValuesSchema extends ValuesSchema,
> = typeof ResourceTypeQueryInput<TValuesSchema>;

export namespace ResourceTypeSelect {
    export interface Props<
        TValuesSchema extends ValuesSchema,
    > extends Omit<
        ComponentProps<ResourceTypeQueryInput<TValuesSchema>>,
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
    return <ResourceTypeQueryInput
        Selector={useCallback(() => <ResourceTypeTable withLinkLock {...tableProps}/>, [])}
        Item={ResourceTypeInline}
        {...props}
    />;
};
