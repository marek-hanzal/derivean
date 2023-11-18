import {ItemSelect}              from "@derivean/item";
import {ProducerIcon}            from "@derivean/ui";
import {NumberInput}             from "@use-pico/form";
import {t}                       from "@use-pico/i18n";
import {
    type ComponentProps,
    type FC
}                                from "react";
import {ProducerSelect}          from "../input/ProducerSelect";
import {ProducerInputComponents} from "../ui/ProducerInputComponents";

export namespace ProducerInputUpsertForm {
    export type Props =
        Omit<
            ComponentProps<ProducerInputComponents["MutationForm"]>,
            "inputs" | "defaultValues" | "Render"
        >
        & {
            producerId?: string
        }
}

export const ProducerInputUpsertForm: FC<ProducerInputUpsertForm.Props> = (
    {
        entity,
        producerId,
        ...props
    }
) => {
    return <ProducerInputComponents.MutationForm
        text={{
            submit:  entity ? t()`Update producer input` : t()`Create producer input`,
            success: entity ? {
                title:   t()`Success`,
                message: t()`Producer input has been updated successfully.`,
            } : {
                title:   t()`Success`,
                message: t()`Producer input has been created successfully.`,
            }
        }}
        icon={<ProducerIcon/>}
        hidden={producerId ? ["producerId"] : undefined}
        inputs={{
            producerId: props => <ProducerSelect
                text={{
                    label:       t()`Producer name`,
                    placeholder: t()`Producer (placeholder)`,
                }}
                {...props}
            />,
            itemId: props => <ItemSelect
                text={{
                    label:       t()`Item name`,
                    placeholder: t()`Item (placeholder)`,
                }}
                {...props}
            />,
            amount:     props => <NumberInput
                label={t()`Consumed amount`}
                {...props}
            />,
        }}
        values={{
            ...entity,
            producerId,
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
            producerId: "",
            itemId: "",
            amount:     0,
        }}
        Render={({Input}) => <>
            <Input name={"producerId"}/>
            <Input name={"itemId"}/>
            <Input name={"amount"}/>
        </>}
        {...props}
    />;
};
