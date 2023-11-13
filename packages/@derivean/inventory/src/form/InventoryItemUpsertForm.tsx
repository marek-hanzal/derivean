import {ItemSelect}        from "@derivean/item";
import {InventoryItemIcon} from "@derivean/ui";
import {NumberInput}       from "@use-pico/form";
import {
    t,
    tx
}                          from "@use-pico/i18n";
import {
    type ComponentProps,
    type FC
}                          from "react";
import {InventorySelect}   from "../input/InventorySelect";
import {InventoryItemUI}   from "../ui/InventoryItemUI";

export namespace InventoryItemUpsertForm {
    export type Props =
        Omit<ComponentProps<InventoryItemUI["MutationForm"]>, "inputs" | "defaultValues" | "Render">
        & {
            inventoryId?: string;
        }
}

export const InventoryItemUpsertForm: FC<InventoryItemUpsertForm.Props> = (
    {
        entity,
        inventoryId,
        ...props
    }
) => {
    return <InventoryItemUI.MutationForm
        text={{
            submit:  entity ? t()`Update inventory item (label)` : t()`Create inventory item (label)`,
            success: {
                title:   t()`Success`,
                message: entity ? t()`InventoryItem updated` : t()`InventoryItem created`,
            }
        }}
        hidden={inventoryId ? ["inventoryId"] : []}
        icon={<InventoryItemIcon/>}
        inputs={{
            inventoryId: props => <InventorySelect
                text={{
                    label:       t()`Inventory`,
                    placeholder: tx()`Inventory (placeholder)`,
                }}
                {...props}
            />,
            itemId:      props => <ItemSelect
                text={{
                    label:       t()`Item`,
                    placeholder: tx()`item (placeholder)`,
                }}
                {...props}
            />,
            amount:      props => <NumberInput
                label={t()`Amount`}
                {...props}
            />,
            limit:       props => <NumberInput
                label={t()`Limit`}
                {...props}
            />,
        }}
        values={{
            ...entity,
            inventoryId,
        }}
        toRequest={values => (entity ? {
            update: {
                update: values,
                query:  {
                    where: {
                        id: entity.id,
                    },
                },
            },
        } : {
            create: values,
        })}
        defaultValues={{
            inventoryId: "",
            itemId:      "",
            amount:      0,
            limit:       0,
        }}
        Render={({Input}) => <>
            <Input name={"inventoryId"}/>
            <Input name={"itemId"}/>
            <Input name={"amount"}/>
            <Input name={"limit"}/>
        </>}
        {...props}
    />;
};
