/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package android.bluetooth.le;

import android.annotation.NonNull;
import android.annotation.Nullable;
import android.os.Parcel;
import android.os.ParcelUuid;
import android.os.Parcelable;
import android.util.ArrayMap;
import android.util.SparseArray;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.Map;
import java.util.Objects;

/**
 * Advertise data packet container for Bluetooth LE advertising. This represents the data to be
 * advertised as well as the scan response data for active scans.
 *
 * <p>Use {@link AdvertiseData.Builder} to create an instance of {@link AdvertiseData} to be
 * advertised.
 *
 * @see BluetoothLeAdvertiser
 * @see ScanRecord
 */
public final class AdvertiseData implements Parcelable {

    @Nullable private final List<ParcelUuid> mServiceUuids;

    @NonNull private final List<ParcelUuid> mServiceSolicitationUuids;

    @Nullable private final List<TransportDiscoveryData> mTransportDiscoveryData;

    private final SparseArray<byte[]> mManufacturerSpecificData;
    private final Map<ParcelUuid, byte[]> mServiceData;
    private final boolean mIncludeTxPowerLevel;
    private final boolean mIncludeDeviceName;
    private final boolean mIncludePublicBroadcastDeviceName;
    private final String mPublicBroadcastDeviceName;
    private final boolean mServiceUuidsEnc;
    private final boolean mServiceSolicitationUuidsEnc;
    private final boolean mTransportDiscoveryDataEnc;
    private final boolean mManufacturerSpecificDataEnc;
    private final boolean mServiceDataEnc;
    private final boolean mTxPowerLevelEnc;
    private final boolean mDeviceNameEnc;
    private final boolean mPublicBroadcastDeviceNameEnc;

    private AdvertiseData(
            List<ParcelUuid> serviceUuids,
            boolean serviceUuidsEnc,
            List<ParcelUuid> serviceSolicitationUuids,
            boolean serviceSolicitationUuidsEnc,
            List<TransportDiscoveryData> transportDiscoveryData,
            boolean transportDiscoveryDataEnc,
            SparseArray<byte[]> manufacturerData,
            boolean manufacturerSpecificDataEnc,
            Map<ParcelUuid, byte[]> serviceData,
            boolean serviceDataEnc,
            boolean includeTxPowerLevel,
            boolean txPowerLevelEnc,
            boolean includeDeviceName,
            boolean deviceNameEnc,
            boolean includePublicBroadcastDeviceName,
            String publicBroadcastDeviceName,
            boolean publicBroadcastDeviceNameEnc) {
        mServiceUuids = serviceUuids;
        mServiceUuidsEnc = serviceUuidsEnc;
        mServiceSolicitationUuids = serviceSolicitationUuids;
        mServiceSolicitationUuidsEnc = serviceSolicitationUuidsEnc;
        mTransportDiscoveryData = transportDiscoveryData;
        mTransportDiscoveryDataEnc = transportDiscoveryDataEnc;
        mManufacturerSpecificData = manufacturerData;
        mManufacturerSpecificDataEnc = manufacturerSpecificDataEnc;
        mServiceData = serviceData;
        mServiceDataEnc = serviceDataEnc;
        mIncludeTxPowerLevel = includeTxPowerLevel;
        mTxPowerLevelEnc = txPowerLevelEnc;
        mIncludeDeviceName = includeDeviceName;
        mDeviceNameEnc = deviceNameEnc;
        mIncludePublicBroadcastDeviceName = includePublicBroadcastDeviceName;
        mPublicBroadcastDeviceName = publicBroadcastDeviceName;
        mPublicBroadcastDeviceNameEnc = publicBroadcastDeviceNameEnc;
    }

    /**
     * Returns a list of service UUIDs within the advertisement that are used to identify the
     * Bluetooth GATT services.
     */
    public List<ParcelUuid> getServiceUuids() {
        return mServiceUuids;
    }

    /**
     * Returns whether Service Uuids need to be encrypted.
     *
     * @hide
     */
    public boolean getServiceUuidsEnc() {
        return mServiceUuidsEnc;
    }

    /**
     * Returns a list of service solicitation UUIDs within the advertisement that we invite to
     * connect.
     */
    @NonNull
    public List<ParcelUuid> getServiceSolicitationUuids() {
        return mServiceSolicitationUuids;
    }

    /**
     * Returns whether Service Solicitation Uuids need to be encrypted.
     *
     * @hide
     */
    public boolean getServiceSolicitationUuidsEnc() {
        return mServiceSolicitationUuidsEnc;
    }

    /** Returns a list of {@link TransportDiscoveryData} within the advertisement. */
    @NonNull
    public List<TransportDiscoveryData> getTransportDiscoveryData() {
        if (mTransportDiscoveryData == null) {
            return Collections.emptyList();
        }
        return mTransportDiscoveryData;
    }

    /**
     * Returns whether Transport Discovery data need to be encrypted.
     *
     * @hide
     */
    public boolean getTransportDiscoveryDataEnc() {
        return mTransportDiscoveryDataEnc;
    }

    /**
     * Returns an array of manufacturer Id and the corresponding manufacturer specific data. The
     * manufacturer id is a non-negative number assigned by Bluetooth SIG.
     */
    public SparseArray<byte[]> getManufacturerSpecificData() {
        return mManufacturerSpecificData;
    }

    /**
     * Returns whether Manufacturer specific data need to be encrypted.
     *
     * @hide
     */
    public boolean getManufacturerSpecificDataEnc() {
        return mManufacturerSpecificDataEnc;
    }

    /** Returns a map of 16-bit UUID and its corresponding service data. */
    public Map<ParcelUuid, byte[]> getServiceData() {
        return mServiceData;
    }

    /**
     * Returns whether Service data need to be encrypted.
     *
     * @hide
     */
    public boolean getServiceDataEnc() {
        return mServiceDataEnc;
    }

    /** Whether the transmission power level will be included in the advertisement packet. */
    public boolean getIncludeTxPowerLevel() {
        return mIncludeTxPowerLevel;
    }

    /**
     * Returns whether Tx Power level needs to be encrypted.
     *
     * @hide
     */
    public boolean getTxPowerLevelEnc() {
        return mTxPowerLevelEnc;
    }

    /** Whether the device name will be included in the advertisement packet. */
    public boolean getIncludeDeviceName() {
        return mIncludeDeviceName;
    }

    /**
     * Whether the public broadcast device name will be included in the advertisement packet.
     * Returns whether Device name needs to be encrypted.
     *
     * @hide
     */
    public boolean getDeviceNameEnc() {
        return mDeviceNameEnc;
    }

    /**
     * Whether the public broadcast device name will be included in the advertisement packet.
     *
     * @hide
     */
    public boolean getIncludePublicBroadcastDeviceName() {
        return mIncludePublicBroadcastDeviceName;
    }

    /**
     * Returns public broadcast name
     * @hide
     */
    public String getPublicBroadcastDeviceName() {
        return mPublicBroadcastDeviceName;
    }

    /**
     * Returns whether Public Broadcast Device Name needs to be encrypted.
     * @hide
     */
    public boolean getPublicBroadcastDeviceNameEnc() {
        return mPublicBroadcastDeviceNameEnc;
    }

    /** @hide */
    @Override
    @SuppressWarnings("ArrayHashCode")
    public int hashCode() {
        return Objects.hash(
                mServiceUuids,
                mServiceUuidsEnc,
                mServiceSolicitationUuids,
                mServiceSolicitationUuidsEnc,
                mTransportDiscoveryData,
                mTransportDiscoveryDataEnc,
                mManufacturerSpecificData,
                mManufacturerSpecificDataEnc,
                mServiceData,
                mServiceDataEnc,
                mIncludeTxPowerLevel,
                mTxPowerLevelEnc,
                mIncludeDeviceName,
                mDeviceNameEnc,
                mIncludePublicBroadcastDeviceName,
                mPublicBroadcastDeviceName,
                mPublicBroadcastDeviceNameEnc);
    }

    /** @hide */
    @Override
    public boolean equals(@Nullable Object obj) {
        if (this == obj) {
            return true;
        }
        if (obj == null || getClass() != obj.getClass()) {
            return false;
        }
        AdvertiseData other = (AdvertiseData) obj;
        return Objects.equals(mServiceUuids, other.mServiceUuids)
                && mServiceUuidsEnc == other.mServiceUuidsEnc
                && Objects.equals(mServiceSolicitationUuids, other.mServiceSolicitationUuids)
                && mServiceSolicitationUuidsEnc == other.mServiceSolicitationUuidsEnc
                && Objects.equals(mTransportDiscoveryData, other.mTransportDiscoveryData)
                && mTransportDiscoveryDataEnc == other.mTransportDiscoveryDataEnc
                && BluetoothLeUtils.equals(
                        mManufacturerSpecificData, other.mManufacturerSpecificData)
                && mManufacturerSpecificDataEnc == other.mManufacturerSpecificDataEnc
                && BluetoothLeUtils.equals(mServiceData, other.mServiceData)
                && mServiceDataEnc == other.mServiceDataEnc
                && mIncludeTxPowerLevel == other.mIncludeTxPowerLevel
                && mTxPowerLevelEnc == other.mTxPowerLevelEnc
                && mIncludeDeviceName == other.mIncludeDeviceName
                && mDeviceNameEnc == other.mDeviceNameEnc
                && mIncludePublicBroadcastDeviceName == other.mIncludePublicBroadcastDeviceName
                && Objects.equals(mPublicBroadcastDeviceName, other.mPublicBroadcastDeviceName)
                && mPublicBroadcastDeviceNameEnc == other.mPublicBroadcastDeviceNameEnc;
    }

    @Override
    public String toString() {
        return "AdvertiseData ["
                + ("mServiceUuids=" + mServiceUuids)
                + (", mServiceUuidsEnc=" + mServiceUuidsEnc)
                + (", mServiceSolicitationUuids=" + mServiceSolicitationUuids)
                + (", mServiceSolicitationUuidsEnc=" + mServiceSolicitationUuidsEnc)
                + (", mTransportDiscoveryData=" + mTransportDiscoveryData)
                + (", mTransportDiscoveryDataEnc=" + mTransportDiscoveryDataEnc)
                + ", mManufacturerSpecificData="
                + BluetoothLeUtils.toString(mManufacturerSpecificData)
                + (", mManufacturerSpecificDataEnc=" + mManufacturerSpecificDataEnc)
                + (", mServiceData=" + BluetoothLeUtils.toString(mServiceData))
                + (", mServiceDataEnc=" + mServiceDataEnc)
                + (", mIncludeTxPowerLevel=" + mIncludeTxPowerLevel)
                + (", mTxPowerLevelEnc=" + mTxPowerLevelEnc)
                + (", mIncludeDeviceName=" + mIncludeDeviceName)
                + (", mDeviceNameEnc=" + mDeviceNameEnc)
                + (", mIncludePublicBroadcastDeviceName=" + mIncludePublicBroadcastDeviceName)
                + mIncludePublicBroadcastDeviceName + ", mPublicBroadcastDeviceName="
                + (", mPublicBroadcastDeviceNameEnc=" + mPublicBroadcastDeviceNameEnc)
                + "]";
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeTypedArray(mServiceUuids.toArray(new ParcelUuid[mServiceUuids.size()]), flags);
        dest.writeByte((byte) (getServiceUuidsEnc() ? 1 : 0));
        dest.writeTypedArray(
                mServiceSolicitationUuids.toArray(new ParcelUuid[mServiceSolicitationUuids.size()]),
                flags);
        dest.writeByte((byte) (getServiceSolicitationUuidsEnc() ? 1 : 0));

        dest.writeTypedList(mTransportDiscoveryData);
        dest.writeByte((byte) (getTransportDiscoveryDataEnc() ? 1 : 0));

        // mManufacturerSpecificData could not be null.
        dest.writeInt(mManufacturerSpecificData.size());
        dest.writeByte((byte) (getManufacturerSpecificDataEnc() ? 1 : 0));
        for (int i = 0; i < mManufacturerSpecificData.size(); ++i) {
            dest.writeInt(mManufacturerSpecificData.keyAt(i));
            dest.writeByteArray(mManufacturerSpecificData.valueAt(i));
        }
        dest.writeInt(mServiceData.size());
        dest.writeByte((byte) (getServiceDataEnc() ? 1 : 0));
        for (ParcelUuid uuid : mServiceData.keySet()) {
            dest.writeTypedObject(uuid, flags);
            dest.writeByteArray(mServiceData.get(uuid));
        }
        dest.writeByte((byte) (getIncludeTxPowerLevel() ? 1 : 0));
        dest.writeByte((byte) (getTxPowerLevelEnc() ? 1 : 0));
        dest.writeByte((byte) (getIncludeDeviceName() ? 1 : 0));
        dest.writeByte((byte) (getDeviceNameEnc() ? 1 : 0));
        dest.writeByte((byte) (getIncludePublicBroadcastDeviceName() ? 1 : 0));
        android.bluetooth.BluetoothUtils.writeStringToParcel(dest, getPublicBroadcastDeviceName());
        dest.writeByte((byte) (getPublicBroadcastDeviceNameEnc() ? 1 : 0));
    }

    public static final @android.annotation.NonNull Parcelable.Creator<AdvertiseData> CREATOR =
            new Creator<AdvertiseData>() {
                @Override
                public AdvertiseData[] newArray(int size) {
                    return new AdvertiseData[size];
                }

                @Override
                public AdvertiseData createFromParcel(Parcel in) {
                    Builder builder = new Builder();
                    ArrayList<ParcelUuid> uuids = in.createTypedArrayList(ParcelUuid.CREATOR);
                    boolean uuidsEnc = (in.readByte() == 1);
                    for (ParcelUuid uuid : uuids) {
                        builder.addServiceUuid(uuid);
                    }
                    if (uuidsEnc) {
                        builder.setServiceUuidEncrypted(uuidsEnc);
                    }

                    ArrayList<ParcelUuid> solicitationUuids =
                            in.createTypedArrayList(ParcelUuid.CREATOR);
                    boolean solicitationUuidsEnc = (in.readByte() == 1);
                    for (ParcelUuid uuid : solicitationUuids) {
                        builder.addServiceSolicitationUuid(uuid);
                    }
                    if (solicitationUuidsEnc) {
                        builder.setSolicitationUuidEncrypted(solicitationUuidsEnc);
                    }

                    List<TransportDiscoveryData> transportDiscoveryData =
                            in.createTypedArrayList(TransportDiscoveryData.CREATOR);
                    boolean tddEnc = (in.readByte() == 1);
                    for (TransportDiscoveryData tdd : transportDiscoveryData) {
                        builder.addTransportDiscoveryData(tdd);
                    }
                    if (tddEnc) {
                        builder.setTransportDiscoveryDataEncrypted(tddEnc);
                    }

                    int manufacturerSize = in.readInt();
                    boolean manufacturerDataEnc = (in.readByte() == 1);
                    for (int i = 0; i < manufacturerSize; ++i) {
                        int manufacturerId = in.readInt();
                        byte[] manufacturerData = in.createByteArray();
                        builder.addManufacturerData(manufacturerId, manufacturerData);
                    }
                    if (manufacturerDataEnc) {
                        builder.setManufacturerDataEncrypted(manufacturerDataEnc);
                    }
                    int serviceDataSize = in.readInt();
                    boolean serviceDataEnc = (in.readByte() == 1);
                    for (int i = 0; i < serviceDataSize; ++i) {
                        ParcelUuid serviceDataUuid = in.readTypedObject(ParcelUuid.CREATOR);
                        byte[] serviceData = in.createByteArray();

                        builder.addServiceData(serviceDataUuid, serviceData);
                    }
                    if (serviceDataEnc) {
                        builder.setServiceDataEncrypted(serviceDataEnc);
                    }
                    builder.setIncludeTxPowerLevel((in.readByte() == 1));
                    boolean includeTxPowerEnc = (in.readByte() == 1);
                    if (includeTxPowerEnc) {
                        builder.setIncludeTxPowerLevelEncrypted(includeTxPowerEnc);
                    }
                    builder.setIncludeDeviceName((in.readByte() == 1));
                    boolean includeDeviceNameEnc = (in.readByte() == 1);
                    if (includeDeviceNameEnc) {
                        builder.setIncludeDeviceNameEncrypted(includeDeviceNameEnc);
                    }
                    builder.setIncludePublicBroadcastDeviceName((in.readByte() == 1), in.readString());
                    boolean includePublicBroadcastDeviceNameEnc = (in.readByte() == 1);
                    if (includePublicBroadcastDeviceNameEnc) {
                        builder.setIncludePublicBroadcastDeviceNameEncrypted(
                                includePublicBroadcastDeviceNameEnc);
                    }

                    return builder.build();
                }
            };

    /** Builder for {@link AdvertiseData}. */
    public static final class Builder {
        @Nullable private List<ParcelUuid> mServiceUuids = new ArrayList<ParcelUuid>();
        @NonNull private List<ParcelUuid> mServiceSolicitationUuids = new ArrayList<ParcelUuid>();

        @Nullable
        private List<TransportDiscoveryData> mTransportDiscoveryData =
                new ArrayList<TransportDiscoveryData>();

        private SparseArray<byte[]> mManufacturerSpecificData = new SparseArray<byte[]>();
        private Map<ParcelUuid, byte[]> mServiceData = new ArrayMap<ParcelUuid, byte[]>();
        private boolean mIncludeTxPowerLevel;
        private boolean mIncludeDeviceName;
        private boolean mIncludePublicBroadcastDeviceName;
        private String mPublicBroadcastDeviceName;
        private boolean mServiceUuidsEnc;
        private boolean mServiceSolicitationUuidsEnc;
        private boolean mTransportDiscoveryDataEnc;
        private boolean mManufacturerSpecificDataEnc;
        private boolean mServiceDataEnc;
        private boolean mTxPowerLevelEnc;
        private boolean mDeviceNameEnc;
        private boolean mPublicBroadcastDeviceNameEnc;

        /**
         * Add a service UUID to advertise data.
         *
         * @param serviceUuid A service UUID to be advertised.
         * @throws IllegalArgumentException If the {@code serviceUuid} is null.
         */
        public Builder addServiceUuid(ParcelUuid serviceUuid) {
            if (serviceUuid == null) {
                throw new IllegalArgumentException("serviceUuid is null");
            }
            mServiceUuids.add(serviceUuid);
            return this;
        }

        /**
         * set the encryption flag for encrypting Service UUID.
         *
         * @param enableEncryption enables encryption for service uuid
         * @hide
         */
        public Builder setServiceUuidEncrypted(boolean enableEncryption) {
            mServiceUuidsEnc = enableEncryption;
            return this;
        }

        /**
         * Add a service solicitation UUID to advertise data.
         *
         * @param serviceSolicitationUuid A service solicitation UUID to be advertised.
         * @throws IllegalArgumentException If the {@code serviceSolicitationUuid} is null.
         */
        @NonNull
        public Builder addServiceSolicitationUuid(@NonNull ParcelUuid serviceSolicitationUuid) {
            if (serviceSolicitationUuid == null) {
                throw new IllegalArgumentException("serviceSolicitationUuid is null");
            }
            mServiceSolicitationUuids.add(serviceSolicitationUuid);
            return this;
        }

        /**
         * set the encryption flag for encrypting solicitation UUID.
         *
         * @param enableEncryption enables encryption for solicitation uuid
         * @hide
         */
        @NonNull
        public Builder setSolicitationUuidEncrypted(boolean enableEncryption) {
            mServiceSolicitationUuidsEnc = enableEncryption;
            return this;
        }

        /**
         * Add service data to advertise data.
         *
         * @param serviceDataUuid 16-bit UUID of the service the data is associated with
         * @param serviceData Service data
         * @throws IllegalArgumentException If the {@code serviceDataUuid} or {@code serviceData} is
         *     empty.
         */
        public Builder addServiceData(ParcelUuid serviceDataUuid, byte[] serviceData) {
            if (serviceDataUuid == null || serviceData == null) {
                throw new IllegalArgumentException("serviceDataUuid or serviceDataUuid is null");
            }
            mServiceData.put(serviceDataUuid, serviceData);
            return this;
        }

        /**
         * set the encryption flag for encrypting service data.
         *
         * @param enableEncryption enables encryption for service data
         * @hide
         */
        public Builder setServiceDataEncrypted(boolean enableEncryption) {
            mServiceDataEnc = enableEncryption;
            return this;
        }

        /**
         * Add Transport Discovery Data to advertise data.
         *
         * @param transportDiscoveryData Transport Discovery Data, consisting of one or more
         *     Transport Blocks. Transport Discovery Data AD Type Code is already included.
         * @throws IllegalArgumentException If the {@code transportDiscoveryData} is empty
         */
        @NonNull
        public Builder addTransportDiscoveryData(
                @NonNull TransportDiscoveryData transportDiscoveryData) {
            if (transportDiscoveryData == null) {
                throw new IllegalArgumentException("transportDiscoveryData is null");
            }
            mTransportDiscoveryData.add(transportDiscoveryData);
            return this;
        }

        /**
         * Set the encryption flag for encrypting Transport Discovery Data.
         *
         * @param enableEncryption enables encryption for Transport Discovery data
         * @hide
         */
        @NonNull
        public Builder setTransportDiscoveryDataEncrypted(boolean enableEncryption) {
            mTransportDiscoveryDataEnc = enableEncryption;
            return this;
        }

        /**
         * Add manufacturer specific data.
         *
         * <p>Please refer to the Bluetooth Assigned Numbers document provided by the <a
         * href="https://www.bluetooth.org">Bluetooth SIG</a> for a list of existing company
         * identifiers.
         *
         * @param manufacturerId Manufacturer ID assigned by Bluetooth SIG.
         * @param manufacturerSpecificData Manufacturer specific data
         * @throws IllegalArgumentException If the {@code manufacturerId} is negative or {@code
         *     manufacturerSpecificData} is null.
         */
        public Builder addManufacturerData(int manufacturerId, byte[] manufacturerSpecificData) {
            if (manufacturerId < 0) {
                throw new IllegalArgumentException("invalid manufacturerId - " + manufacturerId);
            }
            if (manufacturerSpecificData == null) {
                throw new IllegalArgumentException("manufacturerSpecificData is null");
            }
            mManufacturerSpecificData.put(manufacturerId, manufacturerSpecificData);
            return this;
        }

        /**
         * Set the encryption flag for encrypting manufacturer specific data.
         *
         * @param enableEncryption enables encryption for manufacturer data
         * @hide
         */
        public Builder setManufacturerDataEncrypted(boolean enableEncryption) {
            mManufacturerSpecificDataEnc = enableEncryption;
            return this;
        }

        /**
         * Whether the transmission power level should be included in the advertise packet. Tx power
         * level field takes 3 bytes in advertise packet.
         */
        public Builder setIncludeTxPowerLevel(boolean includeTxPowerLevel) {
            mIncludeTxPowerLevel = includeTxPowerLevel;
            return this;
        }

        /**
         * Set the encryption flag for encrypting Include Tx Power Level.
         *
         * @param enableEncryption enables encryption for Include Tx Power Level
         * @hide
         */
        public Builder setIncludeTxPowerLevelEncrypted(boolean enableEncryption) {
            mTxPowerLevelEnc = enableEncryption;
            return this;
        }

        /** Set whether the device name should be included in advertise packet. */
        public Builder setIncludeDeviceName(boolean includeDeviceName) {
            mIncludeDeviceName = includeDeviceName;
            return this;
        }

        /**
         * Set whether the public broadcast device name should be included in advertise packet.
         * Set the encryption flag for encrypting Device Name.
         *
         * @param enableEncryption enables encryption for Device Name
         * @hide
         */
        public Builder setIncludeDeviceNameEncrypted(boolean enableEncryption) {
            mDeviceNameEnc = enableEncryption;
            return this;
        }

        /**
         * Set whether the public broadcast device name should be included in advertise packet.
         *
         * @hide
         */
        @NonNull
        public Builder setIncludePublicBroadcastDeviceName(boolean includeDeviceName) {
            return setIncludePublicBroadcastDeviceName(includeDeviceName, null);
        }

        /**
         * Set whether the public broadcast device name should be included in advertise packet.
         * @hide
         */
        @NonNull
        public Builder setIncludePublicBroadcastDeviceName(boolean includeDeviceName, String pubBroadcastName) {
            mIncludePublicBroadcastDeviceName = includeDeviceName;
            mPublicBroadcastDeviceName = pubBroadcastName;
            return this;
        }

        /**
         * Set the encryption flag for public broadcast device name.
         *
         * @param enableEncryption enables encryption for public broadcast device name
         * @hide
         */
        @NonNull
        public Builder setIncludePublicBroadcastDeviceNameEncrypted(boolean enableEncryption) {
            mPublicBroadcastDeviceNameEnc = enableEncryption;
            return this;
        }

        /** Build the {@link AdvertiseData}. */
        public AdvertiseData build() {
            return new AdvertiseData(
                    mServiceUuids,
                    mServiceUuidsEnc,
                    mServiceSolicitationUuids,
                    mServiceSolicitationUuidsEnc,
                    mTransportDiscoveryData,
                    mTransportDiscoveryDataEnc,
                    mManufacturerSpecificData,
                    mManufacturerSpecificDataEnc,
                    mServiceData,
                    mServiceDataEnc,
                    mIncludeTxPowerLevel,
                    mTxPowerLevelEnc,
                    mIncludeDeviceName,
                    mDeviceNameEnc,
                    mIncludePublicBroadcastDeviceName,
                    mPublicBroadcastDeviceName,
                    mPublicBroadcastDeviceNameEnc);
        }
    }
}
